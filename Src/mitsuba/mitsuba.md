# Mitsuba Parser 移植指南（迁移到其他引擎）

> 目标：将 Mitsuba 的 XML 场景解析前端（parser）最小成本移植到你的渲染/内容引擎，只保留必要功能并保证可维护性。

---
## 目录
1. [总览](#1-总览)
2. [解析阶段架构要点](#2-解析阶段架构要点)
3. [需要移植的最小核心集合 (MVP)](#3-需要移植的最小核心集合-mvp)
4. [依赖与替换策略](#4-依赖--替换策略)
5. [数据结构与类型适配](#5-数据结构与类型适配)
6. [逐模块迁移步骤](#6-逐模块迁移步骤)
7. [解析流程执行顺序 (调用链)](#7-解析流程执行顺序-调用链)
8. [参数替换 ($param) 机制复刻](#8-参数替换-param-机制复刻)
9. [`<include>` 机制与文件解析深度控制](#9-include-机制与文件解析深度控制)
10. [变换 (transform) 解析与数学库替换](#10-变换-transform-解析与数学库替换)
11. [属性系统 (Properties) 的最小实现裁剪](#11-属性系统-properties-的最小实现裁剪)
12. [ObjectType / 插件体系替换建议](#12-objecttype--插件体系替换建议)
13. [光谱 / 颜色 / 纹理处理策略](#13-光谱--颜色--纹理处理策略)
14. [版本升级与可选 transform_* Pass](#14-版本升级与可选-transform_-pass)
15. [错误报告与调试 (line/col)](#15-错误报告与调试-linecol)
16. [性能与增量优化点](#16-性能与增量优化点)
17. [常见陷阱与边界案例](#17-常见陷阱与边界案例)
18. [单元测试与回归测试建议](#18-单元测试与回归测试建议)
19. [分阶段迁移路线图](#19-分阶段迁移路线图)
20. [迁移完成后的可选增强](#20-迁移完成后的可选增强)
21. [全流程 Checklist](#21-全流程-checklist)
22. [示例伪代码](#示例伪代码-核心递归)
23. [补充说明](#补充说明)

---
## 1. 总览
Mitsuba 的 parser 分三阶段：
1. **Parsing**: XML / dict → 中间表示 (`SceneNode` + `Properties`)
2. **Transform passes**: 升级 / 引用解析 / 去重 / 网格合并
3. **Instantiation**: 解析 IR → 实际引擎对象

> 移植到其他引擎，通常只需阶段 (1) + 可选少量 pass；最终用你自己的对象工厂替换 `instantiate()`。

---
## 2. 解析阶段架构要点
核心文件：`src/core/parser.cpp` 与 `include/mitsuba/core/parser.h`

调用链：
```
parse_file / parse_string
  → parse_file_impl
    → parse_xml_node (递归)
       → parse_transform_node (仅 transform 子树)
```
支持特性：
- 参数替换 (`$name`) 与 `<default>`
- 属性节点：`float/integer/boolean/string/point/vector/rgb/spectrum/transform`
- 对象节点：`scene/shape/bsdf/texture/emitter/sensor/sampler/integrator/medium/phase/volume/film/rfilter`
- 引用：`<ref id="..." name="..."/>`
- ID 注册与 `<alias>`
- 嵌套 `<include>`（深度限制）
- 资源路径 `<path>` 注入搜索路径
- 组合变换（左乘：`新操作 * 当前`）
- 精确错误定位（byte offset → 行列）

---
## 3. 需要移植的最小核心集合 (MVP)
**建议首批实现模块：**
A. 结构：`ParserConfig`, `SceneNode`, `ParserState`
B. 基础：`TagType`, `interpret_tag()`
C. 递归解析：`parse_file_impl()`, `parse_xml_node()`, `parse_transform_node()`
D. 参数替换：`make_sorted_parameters()`, `substitute_parameters()`
E. 向量解析：`parse_vector()`, `parse_vector_from_string()`, `expand_value_to_xyz()`
F. 错误包装：`fail()`, `log_with_node()`, `file_location()`
G. 特殊标签：`<include>`, `<ref>`, `<alias>`, `<default>`, `<path>`
H. 变换语义：`translate/rotate/scale/lookat/matrix`
I. `Properties` 最小变体

可暂缓：`transform_upgrade / merge_equivalent / merge_meshes / reorder / relocate / spectrum` 文件读取。

---
## 4. 依赖 & 替换策略
| 原依赖 | 作用 | 替换建议 |
| ------ | ---- | -------- |
| pugixml | XML DOM | 可直接保留；或 tinyxml2（需 API 改） |
| drjit | 数学向量矩阵 | 换 glm/Eigen/自研；提供 Vector3/Matrix4/Affine |
| FileResolver | 路径栈 | 用 `std::vector<std::filesystem::path>` + 线性搜索 |
| Logger/Throw | 日志/异常 | 映射到你引擎的日志系统 |
| Properties | 属性容器 | 实现最小 `std::variant` 版本 |
| util::Version | 版本解析 | 不需要升级可 stub |

---
## 5. 数据结构与类型适配
`SceneNode`：
- `type`, `file_index`, `offset`, `props`

`ParserState`：
- `nodes`, `id_to_index`
- `files`, `versions`（可选）
- `depth`, `content`
- `node_paths`（dict parser 可忽略）

---
## 6. 逐模块迁移步骤
1. 复制/精简 `TagType` 与 `interpret_tag()`（只保留需要标签）
2. 搭建 `Properties`（见第 11 节）
3. 实现 `ParserConfig`（保留 `max_include_depth` / `unused_parameters`）
4. 实现参数替换（最长优先 + 反斜杠转义）
5. 搭建 `parse_file()` / `parse_string()` 外壳
6. 实现 `parse_xml_node()`：
    - 根节点判定 `is_root`
    - 遍历属性执行替换
    - `interpret_tag` 分类
    - 属性节点直接写父 `props`
    - 对象节点创建 `SceneNode` + 注册 ID + 回填引用
    - 处理：`transform/include/ref/alias/default/path`
7. `parse_transform_node()`：严格左乘顺序
8. 引用 `<ref>`：存 `Reference` / 后续解析为 `ResolvedReference`
9. `<alias>`：新 ID → 来源 index
10. `<default>`：仅当未提供该参数
11. `<path>`：追加到搜索路径栈
12. 行列定位：`offset_debug()` + 文件片段扫描

---
## 7. 解析流程执行顺序 (调用链)
```
parse_file()
  → parse_file_impl()
      1. 深度/存在性检查
      2. pugixml 加载文档
      3. root 需有 version
      4. 初始化 state（files + versions）
      5. parse_xml_node(root)

parse_xml_node():
  - 参数替换
  - 解释标签
  - 属性 → 写父 props
  - 对象 → 新 node，父引用，递归子节点
  - 特殊：transform/include/ref/alias/default/path
```

---
## 8. 参数替换 ($param) 机制复刻
**规则**：
- `$name` 最长匹配优先（避免 `$foo` vs `$foobar` 冲突）
- 支持转义：`\$var` → 字面值 `$var`
- 未定义参数立即 `fail()`
- `<default>` 只在原表缺失时插入
- 使用后标记 `used`；结束检查未使用

**实现步骤**：
1. 构建 `SortedParameters`：`key = "$" + name`
2. 按 key 长度降序排序
3. 遍历属性执行替换；二次扫描剩余 `$` 报错
4. 清理 `\$` → `$`

---
## 9. `<include>` 机制与文件解析深度控制
要点：
- 共享同一参数表（默认参数可向下补）
- 深度 +1；超过 `max_include_depth` 抛错
- 解析后合并：`nodes/files/versions/id_to_index`
- 若子文件根为 `<scene>` 跳过其 root（直接挂载其子）
- `ResolvedReference` 索引整体偏移：`+ node_offset - skip`

---
## 10. 变换 (transform) 解析与数学库替换
最终：`T = Op_n * ... * Op_2 * Op_1 * I`
支持：
- `<translate x y z>`
- `<scale x y z>`
- `<rotate angle=... x y z>`（轴不能为零）
- `<lookat origin target up?>`（`up` 缺省自动生成）
- `<matrix value="9|16 values">`（3x3 填充为 4x4）

存储：统一为 `AffineTransform` / `Matrix4`。

---
## 11. 属性系统 (Properties) 的最小实现裁剪
必要类型：
- `Float`, `Integer`, `Boolean`, `String`
- `Vector3`, `Color3`, `Transform`
- `Spectrum`(可 stub)
- `Reference`(待解析 ID), `ResolvedReference`(索引)
- `PluginName`, `Id`

实现：
```cpp
using Variant = std::variant<double,int64_t,bool,std::string,
                             Vector3, Color3, Transform,
                             Reference, ResolvedReference, SpectrumStub>;
```
基本接口：
- `set(name, value, overwrite=true)`
- `get<T>(name)`
- `has_property(name)`
- `rename_property(old,new)`

---
## 12. ObjectType / 插件体系替换建议
若你的引擎对象分类不一致：
- 精简 `ObjectType` 但保留映射稳定
- `interpret_tag()` 删除不支持 case
- `<spectrum type="...">`（Mitsuba 作为纹理）可重映射到你引擎的材质/纹理类型

---
## 13. 光谱 / 颜色 / 纹理处理策略
简化策略：
- `<rgb>` → `Color3`
- `<spectrum value="...">`：若只需常量，1 值 → (v,v,v)，3 值按 RGB；或求均值
- `<spectrum filename="...">`：初期可报 *不支持*（留扩展点）

---
## 14. 版本升级与可选 transform_* Pass
升级点（v1 → v2）：
- camelCase → underscore_case
- `diffuse_reflectance` → `reflectance`
- `uoffset/uscale/voffset/vscale` → 合成 `to_uv` 变换
  无历史包袱可跳过 `transform_upgrade()`。

---
## 15. 错误报告与调试 (line/col)
流程：
1. `pugi::xml_node.offset_debug()` 获取字节偏移
2. 打开文件读取（或缓存）统计行列
3. 统一通过 `fail(state,node,...)` 输出：`filename (line x, col y)`

好处：快速定位 XML 语法或数据错误。

---
## 16. 性能与增量优化点
- 参数替换复杂度：O(N * L) 可接受
- include 深度限制防爆
- transform 内联矩阵乘法
- 解析多为 I/O + DOM 遍历，无需初期多线程
- 大文件后续可考虑 SAX / streaming（高复杂度）

---
## 17. 常见陷阱与边界案例
- 重复 ID / include 中 ID 冲突
- `<alias>` 目标已存在或源不存在
- 非法 transform 子标签（如 `<float>`）
- `<rotate>` 轴为零向量
- `<spectrum>` 同时出现 `value` 与 `filename`
- `<path>` 出现在非 root
- 未解析 `$param`
- 未使用参数（视配置警告/报错）
- `<matrix>` 数量非 9/16
- `<lookat>` origin == target 生成 NaN
- include 循环（深度拦截）

---
## 18. 单元测试与回归测试建议
建议最小测试集合：
1. 基础属性：float/integer/boolean/string/rgb/vector
2. transform 组合：translate+rotate+scale, lookat, matrix9, matrix16
3. 参数替换：基本 / 嵌套 / 转义 `\$` / 未定义参数报错
4. include：正常 / 深度上限 / scene root skip / ID 冲突
5. 引用：`<ref>` 前向 & 后向；`<alias>` 链
6. 错误定位：构造错误 token 验证行列
7. `<default>`：已存在/缺失分支
8. `<path>`：插入搜索路径生效

---
## 19. 分阶段迁移路线图
**Phase 1: 基础骨架**
实现 `ParserConfig / SceneNode / ParserState / TagType / interpret_tag`，集成 pugixml

**Phase 2: 属性 + 基础节点**
解析 `float/integer/boolean/string/vector/point/rgb` + object 容器

**Phase 3: transform + 引用**
`translate/rotate/scale/lookat/matrix` + `<ref>` / `<alias>`

**Phase 4: include + 参数替换 + default**
递归 / 深度限制 / index 重映射

**Phase 5: 高级/可选**
`spectrum` 常量 / `path` / 行列定位 / 未使用参数检测

**Phase 6: 实例化适配层**
`plugin_name → 对象工厂` + 依赖顺序

---
## 20. 迁移完成后的可选增强
- JSON / YAML 输入 → 复用 IR
- XML 导出：`write_string / write_file`
- `transform_merge_equivalent / transform_merge_meshes`
- Streaming / SAX 降低峰值内存
- XSD schema 校验预检测结构问题

---
## 21. 全流程 Checklist
- [ ] 定义核心枚举/结构 (SceneNode/ParserState/ParserConfig)
- [ ] 搭建 Properties 简化版
- [ ] 集成 pugixml 并能加载最小 XML
- [ ] interpret_tag 覆盖需要的标签
- [ ] 参数替换 (含默认值 + 转义) 与未使用检查
- [ ] 各属性标签解析 & 类型校验
- [ ] transform 子语法 + 顺序验证
- [ ] 引用/alias/ID 注册冲突处理
- [ ] include 文件合并 & 索引偏移 & 深度限制
- [ ] path 搜索路径注入
- [ ] 错误行列定位
- [ ] 单元测试 1~8 全绿
- [ ] 性能冒烟测试 (大批简单节点)
- [ ] 与引擎对象工厂对接 (instantiate 替换)
- [ ] 代码文档与使用示例

---
## 示例伪代码 (核心递归)
```pseudo
function parse_xml_node(node, parent_index):
  do_parameter_substitution(node.attributes)
  tag = interpret_tag(node.name)
  switch(tag):
    case PROPERTY:
      parent.props.set(name, parse_value(node))
    case OBJECT:
      idx = state.add(SceneNode)
      if parent != root:
        parent.props.set(prop_name, ResolvedReference(idx))
      for child in node.children:
        parse_xml_node(child, idx)
    case TRANSFORM:
      T = I
      for op in node.children:
        apply_op(T, op)
      parent.props.set(name, T)
    case INCLUDE:
      sub_state = parse_file_impl(file)
      merge(sub_state)
    ...
```

---
## 补充说明
1. 变换顺序：保持左乘以匹配 Mitsuba 的期望坐标空间。
2. 引用策略：初期可保留名称引用，后续加 `transform_resolve` 将 name → index。
3. 版本：无兼容需求可跳过 `versions` / `upgrade`。
4. 安全：`<include>` 解析临时修改搜索路径表，结束后恢复（推荐 RAII）。

> 完成以上步骤后，你将拥有一个可复用的 XML 场景前端；最后只需写一个 `instantiate` 或工厂映射层，将 `plugin_name` 映射到引擎内部组件构造逻辑。

---
**结束**
