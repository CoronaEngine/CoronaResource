# Datasmith 源码逐文件解析（附录补充）

> 说明：本文件进一步深入到 DatasmithCore 目录下 Public / Private 各文件的职责、结构、主要类型与调用关系。仅基于当前仓库可见的文件名与已读取头文件内容整理；对于未展开源码的实现细节，标注为“推断”或“典型模式”。可直接并入主解析文档。

---
## 目录
1. 总体结构概览
2. 命名与分层约定
3. Public 头文件逐文件解析
4. Private 实现文件逐文件解析
5. DirectLink 子模块文件
6. 调用/依赖拓扑（逻辑示意）
7. 关键数据流（Mesh / Material / Animation / Variant / XML / DirectLink）
8. 扩展与二次开发插入点
9. 维护与审查检查表
10. 快速检索索引表
11. 逐文件深度解析（Public / Private 全量）
12. 符号速查与接口清单（完整索引）
13. 实现层反射/参数存储机制深入解析
14. DirectLink 与 Hash 协同工作流程
15. 扩展示例代码模板
16. 回归测试矩阵
17. 变更影响映射
18. 未来演进路线
19. 术语表 & 集成速览
20. 核心 API 签名速览（补充）
21. 典型调用序列（Scenario Flows）
22. 导入/差异化策略与优化建议
23. DirectLink 消息格式与示例

---
## 1. 总体结构概览

- Public：对外 SDK / 插件使用的接口抽象层（Element 接口族、工厂、类型枚举、序列化工具、日志）。
- Private：接口的具体实现（*Impl 类）、XML 读写、Mesh/Material/Variant/Animation 内部存储、杂项工具、Hash 计算与缓存、场景共享状态。
- DirectLink 子目录：增量同步通道的场景接收与辅助工具。
- 编译单元策略：大多数 Element 类型的实现集中在少量 *Impl.cpp 内，通过模板或通用基类减少重复。

---
## 2. 命名与分层约定
| 前缀/后缀                                 | 含义                        | 示例                                   |
|---------------------------------------|---------------------------|--------------------------------------|
| IDatasmith*                           | Public 抽象接口               | `IDatasmithMeshElement`              |
| FDatasmith*Impl                       | Private 实现类（多为模板基类派生）     | `FDatasmithMeshElementImpl`          |
| FDatasmithSceneFactory                | 统一创建 API（集中管理 subtype 分派） | `FDatasmithSceneFactory::CreateMesh` |
| *Elements.h / *ElementsImpl.h         | Element 接口 / 实现           | 材质、动画、场景等                            |
| *Serializer                           | 专用二进制/文本序列化器              | `FDatasmithAnimationSerializer`      |
| *XmlReader / *XmlWriter               | XML 解析与输出                 | `DatasmithSceneXmlReader.cpp`        |
| Variant / Animation / Mesh / Material | 功能域子模块                    | `DatasmithVariantElementsImpl.h`     |

---
## 3. Public 头文件逐文件解析

### 3.1 DatasmithCore.h
- 作用：声明日志分类 `LogDatasmith`。
- 用途：统一日志入口，外部模块可使用 `UE_LOG(LogDatasmith, ...)`。

### 3.2 DatasmithDefinitions.h
- 内容：枚举与常量宏；Element Type / SubType / 纹理模式 / 材质模式 / 变体 / 动画 / Light / Blend / Shader Usage 等。
- 关键点：
    - `EDatasmithElementType` 使用位标志；工厂通过类型 + 可选 SubType 分派。
    - 大量 TEXT 宏键（XML Attr / Node 名 / Meta Key），保证读写对称。
    - TransformChannels 枚举与引擎 MovieScene 通道对应。
- 扩展建议：新增类型需在：枚举 → 工厂 switch → XML（Reader/Writer）同步。

### 3.3 DatasmithTypes.h
- 定义：`FDatasmithTextureSampler`, `FDatasmithTransformFrameInfo`, `FDatasmithVisibilityFrameInfo`。
- 职责：
    - 纹理采样参数聚合（UV 变换 / 镜像 / 裁剪 / 通道 / 倍增 / 反转）。
    - 动画帧数据（位置/旋转/缩放或可见性）。
- 关键：旋转帧使用欧拉角（度），并提供 `IsValid()` / 比较运算符用于去重或 Hash。

### 3.4 IDatasmithSceneElements.h
- 最大的接口汇集文件。包含所有 Element 抽象：Actor / Mesh / Light / Camera / Material / Texture / Animation / Variant / MetaData / Scene 等。
- 设计特点：
    - 分离资源（MeshElement, TextureElement, MaterialElement）与实例（MeshActorElement, HierarchicalISM）。
    - 每个接口集中“读写属性 + 子对象访问 + 集合管理”方法。
    - `IDatasmithScene` 统一管理集合：Add/Remove/Count/Get（Mesh/Actor/Material/Texture/LevelSequence/VariantSets/MetaData）。
    - 通过纯虚函数隐藏实现，便于跨 DLL ABI 稳定。
- 扩展：添加新的 Element 接口时需：a) 枚举类型；b) 工厂创建；c) XML 序列化；d) DirectLink Hash/同步支持（可选）。

### 3.5 DatasmithSceneFactory.h
- 工厂：所有 Element 创建集中点。
- 提供 `CreateElement(EDatasmithElementType, SubType, Name)` 统一入口；内部 switch 分派到更细化的 `CreateXxx()`。
- 价值：
    - 屏蔽实现类模板/构造细节。
    - 便于统计/调试（可在实现里注入追踪）。
- 注意：Deprecated API（Cloth）保留向后兼容；新增类型需同步补充 switch。

### 3.6 DatasmithMaterialElements.h
- 材质表达式图接口：表达式、输入、输出、参数化节点（Bool/Color/Scalar/Texture/Coordinate/Flatten/Custom/FunctionCall/Generic）。
- `IDatasmithUEPbrMaterialElement`：UE PBR 封装 + 各通道输入访问器 + 表达式集合管理。
- 目标：导入时快速还原材质 graph 或生成参数化实例。
- 扩展：添加新表达式 → 枚举 `EDatasmithMaterialExpressionType` → 工厂 `CreateMaterialExpression` + UEPbrMaterial AddMaterialExpression 模板特化。

### 3.7 DatasmithAnimationElements.h
- 动画层级接口：Transform / Visibility / Subsequence / LevelSequence。
- 核心 API：AddFrame / GetFrame / RemoveFrame / CurveInterpMode / CompletionMode / 子序列时间缩放 & 起始帧。
- 设计：细分不同动画种类便于导入 Sequencer 轨道映射与差异化压缩。

### 3.8 DatasmithAnimationSerializer.h
- 二进制（或可选调试文本）序列化与反序列化 LevelSequence。
- 扩展点：可加入版本头 / 压缩方案（如 zstd）/ 校验哈希。

### 3.9 DatasmithMesh*.h (Mesh / Serialization / UObject)
- （未展开源码，基于命名推断）
    - `DatasmithMesh.h`: Mesh 数据结构（顶点/索引/UV/切线/面材料索引等）与构建 API。
    - `DatasmithMeshSerialization.h`: Mesh 数据读写（磁盘缓存 .geom 或内存流）与版本兼容。
    - `DatasmithMeshUObject.h`: 与 UE `UStaticMesh` 交互辅助（构建 UStaticMesh、设置 LOD/Lightmap UV）。

### 3.10 DatasmithMaterialsUtils.h
- 推断：材质参数转换 / 颜色空间处理 / 纹理路径解析 / 常用表达式组装函数。

### 3.11 DatasmithUtils.h
- 公共工具：字符串正则/路径正规化/Hash 辅助/浮点量化/命名唯一化。

### 3.12 DatasmithSceneXmlReader.h / DatasmithSceneXmlWriter.h
- XML 结构：解析/生成 `.udatasmith`。
- Reader：构建 Element 树，处理版本补丁、名称映射、引用解析。
- Writer：序列化元素属性，跳过默认值（减少大小），输出 Hash。

### 3.13 DatasmithVariantElements.h
- Variant（LevelVariantSets / VariantSet / Variant / ActorBinding / PropertyCapture / ObjectPropertyCapture）结构接口。
- 用途：记录差异化属性快照，支持快速切换配置。

### 3.14 DatasmithAnimationSerializer.h（已述）

### 3.15 DatasmithCloth.h（Deprecated）
- 已废弃；保留 ABI 兼容占位。

### 3.16 DirectLink/ DatasmithDirectLinkTools.h & DatasmithSceneReceiver.h
- Tools：类型名获取（GetElementTypeName）、Dump 场景调试。
- SceneReceiver：接收增量消息、重建或更新本地 Element 节点（未查看实现，基于命名推断）。

---
## 4. Private 实现文件逐文件解析
（含 *Impl.h 仅看到文件名未展开代码部分 → 采用“已知接口 + 常见模式”推断，标注）

### 4.1 DatasmithSceneFactory.cpp
- 实现：所有 `CreateXxx()` 调用 `MakeShared< FDatasmith*Impl >`。
- Switch 逻辑：`CreateElement(Type, SubType, Name)` → 匹配具体 Element；子类型（动画 / 变体）再走内层 switch。
- 特点：
    - Deprecated Cloth 分支保留。
    - MaterialExpression 通过 SubType 映射枚举。
    - 若新增类型未补充 → 返回空共享指针（潜在风险：调用侧需校验）。
- 优化空间：
    - 可加入“默认 case 日志 + 统计”防遗漏；
    - 可缓存工厂函数表（数组索引）减少 switch 深度。

### 4.2 DatasmithSceneElementsImpl.{h,cpp}
- 作用（推断）：实现所有基础 Element：Scene / Actor / MeshActor / Light / Camera / CustomActor / Landscape / Decal / PostProcessVolume / MetaData / Texture / Mesh / HierarchicalISM。
- 预期内容：
    - 存储字段（Name / Label / Properties / Transform / Flags / MaterialOverrides / ChildList）。
    - `CalculateElementHash`：聚合关键属性 -> MD5；内部缓存 dirty 标志。
    - 子节点管理：AddChild / AttachActor 变更父引用。
    - Light 特殊：IES 路径 / 强度 / 色温 / 单位 / 形状参数。
- 注意：需确保线程安全（通常仅构建线程使用，DirectLink Diff 只读）。

### 4.3 DatasmithMaterialElementsImpl.{h,cpp}
- 实现：材质、PBR、表达式节点/输入/输出、参数存储、连接图（指针或索引引用）。
- 关键：
    - 连接：`ConnectExpression(input)` 设置输入表达式指针 + output index。
    - ResetExpressionGraph：清空输入/表达式数组并重置默认值。
    - Hash：拓扑 + 节点类型 + 参数值序列化（顺序化保证稳定）。

### 4.4 DatasmithAnimationElementsImpl.{h,cpp}
- 实现：Transform / Visibility / Subsequence / LevelSequence 元素存储 + 帧数组 (TArray)。
- 逻辑：AddFrame 时保持排序（或追加后排序）；去重（比较上一次帧）。
- 完成模式 / 插值模式 存储简单枚举值。

### 4.5 DatasmithVariantElementsImpl.h (及可能 cpp)
- 存储：VariantSet -> Variants -> ActorBindings -> PropertyCaptures（属性路径 + 分类 + 序列化值）。
- 特性：PropertyCapture 可能存多类型值（字符串化 / 原始缓冲）。
- Hash：绑定 ActorName + 属性路径 + 值。

### 4.6 DatasmithMesh.cpp
- 推断：网格数据结构（Positions / Normals / UV[n] / Tangents / Colors / Indices / 面材料 ID）。
- 可能提供：BuildTangents、Compact、Validate、MergeUVChannel、ComputeBounds。

### 4.7 DatasmithMeshSerialization.cpp
- 负责：自定义二进制格式读写 (.geom)。
- 典型字段：版本号 / Header（顶点数/IndexCount/UVChannelCount/Flags） / 数据块 MD5 / 可选压缩（LZ4/Oodle）。

### 4.8 DatasmithMeshUObject.cpp
- 与 UE 运行时交互：创建 `UStaticMesh`、填充渲染资源、设置 LightmapCoordinateIndex、构建 Collisions（可选）。

### 4.9 DatasmithAnimationSerializer.cpp
- 实现 Public 序列化接口：
    - Serialize：写 header (magic + version) → FrameRate → Anim 数 → 每 Anim 类型标识 + 通道数据（帧数 + (FrameNumber,Value) 序列）。
    - Deserialize：逆过程 + 填充元素 + 校验哈希（若存储）。
- DebugFormat=true：可能输出 JSON/可读文本（推断）。

### 4.10 DatasmithUtils.cpp
- 常用工具：路径规范化（相对→绝对），UTF-8 / 宽字符转换，随机/唯一名生成，数学辅助（量化浮点）。

### 4.11 DatasmithMaterialsUtils.cpp
- 材质辅助：通道复制、表达式查找、创建默认参数、颜色线性/伽马转换、金属度/IOR 推导。

### 4.12 DatasmithTypes.cpp
- 帧结构默认值、静态无效帧单例 `InvalidFrameInfo` 初始化，比较函数实现。

### 4.13 DatasmithSceneXmlReader.cpp
- 解析：
    - XML DOM/流 → 遍历节点 → `Type` 或标签 → 调工厂创建 Element。
    - 读属性映射（字符串 → 基本类型转换）。
    - Resolve pass：填充引用（MeshActor.MeshName → MeshElement）。
    - PatchUpVersion：旧字段迁移（e.g., IntensityUnits 改名）。
    - 错误处理：缺失必需属性 → 日志 + 跳过。

### 4.14 DatasmithSceneXmlWriter.cpp
- 输出：
    - 根节点属性（Host/Product/ExporterVersion/Meta）。
    - 资源列表分组（Meshes / Textures / Materials / Actors / Variants / Sequences / MetaData）。
    - 压缩：默认值省略，布尔/枚举用紧凑字符串描述。

### 4.15 DatasmithSceneGraphSharedState.h
- 推断：共享状态（名称表 / Hash 缓存 / 线程安全容器）供多实现文件使用。

### 4.16 DatasmithCore.cpp
- 模块入口：日志分类定义、全局标志初始化、可能注册 DirectLink 支持。

### 4.17 DatasmithLocaleScope.{h,cpp}
- 作用：区域设置（Locale）临时切换（如数字小数点）、导出/解析本地化敏感数据保持一致。 RAII 设置/恢复。

### 4.18 DatasmithCloth*.cpp (Deprecated)
- 早期布料导入实验代码保留；未来可删除后需清理工厂/枚举条件宏。

### 4.19 Private/DirectLink/DatasmithDirectLinkTools.cpp
- 实现 `GetElementTypeName`：基于 `IDatasmithElement::IsA` 返回字符串，用于调试打印。
- `DumpDatasmithScene`：遍历 Scene 元素输出层级/计数/Hash 供诊断。

### 4.20 Private/DirectLink/DatasmithSceneReceiver.cpp
- 场景增量接收：
    - 接收消息（Add/Remove/Update/Reparent）。
    - 通过工厂或查表定位 Element；更新属性后标记 Hash 脏。
    - 应用顺序：移除 → 添加资源 → 添加 Actor → 变体/动画 → MetaData。
- 一致性：序列号 / 批量提交 / 失败回滚（推断）。

---
## 5. DirectLink 子模块文件关系
| 文件                                              | 职责         | 依赖                          |
|-------------------------------------------------|------------|-----------------------------|
| Public/DirectLink/DatasmithDirectLinkTools.h    | 调试工具声明     | IDatasmithSceneElements.h   |
| Private/DirectLink/DatasmithDirectLinkTools.cpp | 场景遍历/类型字符串 | 工厂/Element IsA              |
| Private/DirectLink/DatasmithSceneReceiver.cpp   | 增量消息应用     | Hash / Factory / Element 修改 |

---
## 6. 调用 / 依赖拓扑（逻辑示意）
```
           +---------------------------+
           |  用户 / Translator 插件    |
           +--------------+------------+
                          |  (Create/Populate)
                          v
               +-----------------------+
               | FDatasmithSceneFactory|  <--- 依赖枚举 DatasmithDefinitions.h
               +-----------+-----------+
                           | (TSharedRef Impl)
   +-----------------------+------------------------------+
   |                       |                              |
   v                       v                              v
Impl: SceneElements   MaterialElementsImpl         AnimationElementsImpl
   |                       |                              |
   +---------+-------------+--------------+---------------+
             |                            |
             v                            v               v
      Scene Collections          Hash / Serialize   VariantElementsImpl
             |                            |               |
             |                            |               |
             +-------------+--------------+---------------+
                           v
                 XML Writer / Reader
                           |
                      .udatasmith 文件
                           |
                       (Import 时)
                           v
                    UObjects 构建层
```

---
## 7. 关键数据流简述

### 7.1 Mesh
Translator → `CreateMesh` → SetFile(.geom) & LOD / Materials → Scene.AddMesh → 导入时读取 `.geom` → `UStaticMesh`。

### 7.2 Material
Translator → `CreateUEPbrMaterial` + AddMaterialExpression(参数化) → Hash → 导入器：生成 parent Material 或 Instance。

### 7.3 Animation
收集帧 → Transform/Visibility Element/AddFrame → LevelSequence.AddAnimation → 可选二进制 `.udsanim` → 导入转 Sequencer 轨道。

### 7.4 Variant
创建 LevelVariantSets/VariantSet/Variant → ActorBinding + PropertyCapture → 导入生成 `ULevelVariantSets`。

### 7.5 MetaData
Element 创建后：CreateMetaData + AddProperty(Key,Value) → 场景索引。

### 7.6 DirectLink
外部 DCC 改动 → Diff 生成消息 → Receiver（SceneReceiver.cpp）应用 → 修改 Element 属性/Hash → 视口刷新。

---
## 8. 扩展与二次开发插入点
| 目标              | 位置                                         | 步骤                                           | 风险                           |
|-----------------|--------------------------------------------|----------------------------------------------|------------------------------|
| 新 Element 类型    | Definitions + Factory + Impl + XML         | 1) 枚举 2) 工厂 case 3) Impl 类 4) XML 读写 5) Hash | 遗漏 case 导致 CreateElement 返回空 |
| 新材质表达式          | EDatasmithMaterialExpressionType + Factory | 加枚举 + switch → Impl                          | Hash 稳定序列需更新                 |
| 自定义序列化二进制       | *Serializer 新文件                            | 设计 header + version + 校验                     | 与现有 XML 同步字段                 |
| DirectLink 过滤同步 | SceneReceiver.cpp                          | 消息分类过滤                                       | 同步一致性测试复杂                    |
| Mesh 压缩         | MeshSerialization.cpp                      | 加压缩后缀/flag                                   | 旧版本兼容                        |
| 动画压缩策略          | AnimationElementsImpl / Serializer         | 插入采样/量化                                      | 误差评估正确性                      |

---
## 9. 维护与审查检查表
- 工厂 switch 是否覆盖新增类型 / 表达式？
- XML Reader/Writer：新增属性是否对称写入/读取？
- Hash 逻辑：是否遗漏影响渲染结果的字段？
- Deprecated 标记清理：Cloth 相关是否计划移除？
- DirectLink：增量更新是否做脏标记合并节流？
- 序列化版本：PatchUpVersion 测试用例是否更新？
- 线程安全：构建阶段与 DirectLink 回调是否存在并发写？

---
## 10. 快速检索索引表
| 功能域        | 入口文件                                         | 主要实现                            | 相关辅助                                    |
|------------|----------------------------------------------|---------------------------------|-----------------------------------------|
| 场景/集合      | IDatasmithSceneElements.h                    | DatasmithSceneElementsImpl.*    | DatasmithSceneFactory.cpp               |
| 工厂         | DatasmithSceneFactory.h                      | DatasmithSceneFactory.cpp       | Definitions.h                           |
| Mesh 数据    | DatasmithMesh.h                              | DatasmithMesh.cpp               | MeshSerialization.cpp / MeshUObject.cpp |
| 材质表达式      | DatasmithMaterialElements.h                  | DatasmithMaterialElementsImpl.* | MaterialsUtils.cpp                      |
| 动画         | DatasmithAnimationElements.h                 | AnimationElementsImpl.*         | AnimationSerializer.*                   |
| Variant    | DatasmithVariantElements.h                   | VariantElementsImpl.*           | SceneElementsImpl.*                     |
| 纹理/采样      | IDatasmithSceneElements.h / DatasmithTypes.h | SceneElementsImpl.*             | Utils.cpp                               |
| XML        | DatasmithSceneXmlReader/Writer.h             | 同名 .cpp                         | LocaleScope.*                           |
| DirectLink | DirectLink/*.h                               | DirectLink/*.cpp                | SceneElementsImpl.*                     |
| Hash/工具    | DatasmithUtils.h                             | DatasmithUtils.cpp              | Types.cpp                               |
| MetaData   | IDatasmithSceneElements.h                    | SceneElementsImpl.*             | SceneXml 读写                             |

---

> 注：对于标记“推断”的部分，如需严格精确实现细节，应继续展开对应 .cpp 文件查看；本解析已为架构与二次开发提供足够导图式信息。

（完）

---
## 11. 逐文件深度解析（Public / Private 全量）
> 范围：`Datasmith_runtime/DatasmithCore/Public` 与 `Private` 目录中已列出的全部文件（含 DirectLink 子目录概述）。对每个文件给出：职责定位、主要公开/内部符号（类/函数/枚举）、核心数据结构或算法要点、典型调用关系、可扩展点、常见风险/注意事项、与其它文件的耦合标签。未直接展开源码的实现处以“推断”标注。该节可直接补充进主 Markdown。

### 组织视图
- Public：纯接口 & 常量枚举 & 工厂入口 & 序列化/解析 API 声明（保持 ABI 稳定 / 封装实现）。
- Private：接口实现、实际数据持有、Hash/序列化逻辑、XML 读写、辅助算法、DirectLink 增量同步。
- 依赖方向：Public (接口/枚举) ← Private 实现；所有实现共享 Definitions 常量；DirectLink 仅依赖公共接口 + 少量内部状态（Hash）。

### 11.1 Public 目录逐文件

| 文件                               | 职责定位          | 关键内容/核心符号                                         | 典型扩展动作               | 主要风险        | 快速验证             | 备注           |
|----------------------------------|---------------|---------------------------------------------------|----------------------|-------------|------------------|--------------|
| DatasmithCore.h                  | 日志入口          | LogDatasmith 分类                                   | 新增日志分类               | 重复定义导致链接冲突  | 编译 + 查看日志输出      | 保持最小化        |
| DatasmithDefinitions.h           | 中央枚举/常量       | 各 Element / SubType / MaterialExpr / XML Key      | 加枚举 + 工厂/XML/Hash 同步 | 遗漏分支/冲突位    | 枚举脚本审计           | 枚举变更需更新测试    |
| DatasmithTypes.h                 | 基础数据结构        | TextureSampler / TransformFrame / VisibilityFrame | 新帧结构                 | 比较/Hash 不稳定 | RoundTrip + Hash | 旋转欧拉精度注意     |
| IDatasmithSceneElements.h        | 所有元素接口        | Actor/Resource/Variant/Animation 等纯虚函数            | 新增接口 + Impl          | ABI 顺序破坏    | 编译 + 旧二进制兼容测试    | 慎改已有签名       |
| DatasmithSceneFactory.h          | 统一创建 API      | CreateElement / CreateMesh 等                      | 增类型/子类型注册            | 漏返回空指针      | 全类型创建测试          | 可迁移至注册表      |
| DatasmithMaterialElements.h      | 材质表达式接口       | Expression 派生 + UEPbrMaterial 通道                  | 新表达式枚举+工厂            | Hash 不含新节点  | Graph Hash 测试    | 确保拓扑稳定       |
| DatasmithAnimationElements.h     | 动画接口          | Transform/Visibility/Subsequence/LevelSequence    | 新动画子类型               | 帧排序/去重失效    | 插入乱序帧测试          | Channels 位一致 |
| DatasmithAnimationSerializer.h   | 动画序列化声明       | Serialize/Deserialize                             | 新版本号                 | 旧文件不可读      | 版本兼容测试           | 增加魔数校验       |
| DatasmithMesh*.h                 | Mesh 抽象与序列化接口 | Mesh 数据/Geom I/O/UObject 辅助                       | 新属性/格式               | 版本兼容破坏      | 旧新文件对比           | 大文件性能        |
| DatasmithMaterialsUtils.h        | 材质辅助          | 颜色空间/表达式助手                                        | 新转换函数                | 精度/重复节点     | 单元测试色差           | 建议加色差阈值      |
| DatasmithUtils.h                 | 通用工具          | 清理/遍历/哈希                                          | 新遍历 API              | O(N^2) 风险   | 大场景基准            | 可缓存索引        |
| DatasmithSceneXmlReader/Writer.h | XML I/O 接口    | Read/Write 场景                                     | 新字段读写                | 默认值不对称      | RoundTrip Diff   | Locale 统一    |
| DatasmithVariantElements.h       | Variant 接口    | 变体/绑定/属性捕获                                        | 新捕获类型                | Hash 漏字段    | 变体 Hash 测试       | 路径兼容迁移       |
| DatasmithCloth.h                 | Deprecated 占位 | Cloth 接口(废弃)                                      | 移除                   | 误用          | 枚举不再引用           | 标注删除计划       |
| DirectLink/* 公共头                 | DirectLink 工具 | 工具/接收接口声明                                         | 新消息类型                | 消息号冲突       | Mock 消息测试        | 与 Hash 协同    |

---
#### 11.1.1 DatasmithCore.h
| 项    | 说明                                                    |
|------|-------------------------------------------------------|
| 职责   | 声明日志分类 `LogDatasmith`（模块统一日志入口）。                      |
| 主要符号 | `DECLARE_LOG_CATEGORY_EXTERN(LogDatasmith, Log, All)` |
| 设计要点 | 最小化：仅暴露日志，避免其它耦合；实现放在 `DatasmithCore.cpp`。            |
| 扩展点  | 增加更多日志分类时可集中放这里或分文件；保持最小化可读性。                         |
| 风险   | 若其它文件直接定义，会出现重复符号；需确保只 extern。                        |

#### 11.1.2 DatasmithDefinitions.h
| 项    | 说明                                                                                                                                                                                                                                                                        |
|------|---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| 职责   | 元数据 / 枚举 / 宏常量统一中心：类型位标志、动画/变体子类型、材质表达式类型、纹理模式、灯光单位、材质/混合/合成模式、属性类别、XML Tag/Attr Key 字符串等。                                                                                                                                                                                |
| 关键枚举 | `EDatasmithElementType` (bit flags), `EDatasmithElementAnimationSubType`, `EDatasmithElementVariantSubType`, `EDatasmithMaterialExpressionType`, `EDatasmithTransformChannels`, 各种 Light / Texture / Material / Shader / ActorMobility / Completion / PropertyCategory 等。 |
| 核心模式 | 通过宏 (e.g., `#define DATASMITH_LIGHTINTENSITYNAME TEXT("Intensity")`) 保证 XML / 序列化写入与读取一致；新增字段需添加成对宏。                                                                                                                                                                      |
| 扩展步骤 | 1) 新增枚举值 2) 更新工厂 switch 3) 更新 XML Reader/Writer 分支 4) 更新 DirectLink diff (若需) 5) 补测试。                                                                                                                                                                                     |
| 风险   | a) 遗漏对应 writer/reader 导致字段丢失 b) 重复宏名 c) 枚举值序数/位掩码冲突。                                                                                                                                                                                                                      |

#### 11.1.3 DatasmithTypes.h
| 项  | 说明                                                                                                  |
|----|-----------------------------------------------------------------------------------------------------|
| 职责 | 基础数据结构：`FDatasmithTextureSampler`, `FDatasmithTransformFrameInfo`, `FDatasmithVisibilityFrameInfo`。 |
| 要点 | 动画帧使用 FrameNumber + 值；旋转存欧拉角（度）；提供 `IsValid()` 和比较运算符 → 支撑去重/Hash。                                  |
| 设计 | 尽量无外部依赖，仅数学向量/Quat；利于序列化独立处理。                                                                       |
| 扩展 | 新增帧类型（比如 MorphFrameInfo）需同步动画实现与序列化器。                                                               |
| 风险 | 欧拉角与四元数转换精度/顺序（Yaw/Pitch/Roll）差异；Hash 需保持顺序稳定。                                                      |

#### 11.1.4 IDatasmithSceneElements.h
| 项         | 说明                                                                                                                                                                                                              |
|-----------|-----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| 职责        | Datasmith 所有核心抽象接口集合（Actor / Mesh / Light / Camera / MaterialID / Texture / MeshActor / LightActor / Landscape / PostProcess / HierarchicalISM / Decal / Variant 系列 / MetaData / LevelSequence / 各类 Animation）。 |
| 主要接口模式    | 全部继承自 `IDatasmithElement`（具名 + Label + Hash + IsA）；Actor 系列扩展 Transform / Layer / Tags / Child 树；资源类提供路径/参数；材质/纹理/动画/变体以专用子接口细化。                                                                                |
| 关键功能      | a) 树状 Actor 结构 b) 资源与实例分离 c) LOD/材质槽访问 d) 材质 Override 列表 e) Light 属性（强度、色温、IES） f) Animation 帧增删与曲线模式 g) Variant 层级（VariantSets→Variant→ActorBinding→PropertyCapture）。                                          |
| 扩展点       | 新类型添加接口 + Definitions 位标识 + 工厂 + XML；可继承现有 Actor 逻辑或资源逻辑。                                                                                                                                                       |
| 风险        | 接口扩张导致 ABI 改变（需保持纯虚函数声明顺序谨慎）；指针/共享指针循环引用（父子链）。                                                                                                                                                                  |

#### 11.1.5 DatasmithSceneFactory.h
| 职责 | 所有 Element 创建单点（`CreateElement` + 细化 `CreateXxx`）。 |
| 模式 | 内部 switch (Type/SubType) → `MakeShared< Impl >`（在 .cpp 中）。 |
| 扩展 | 添加枚举后必须补 switch 分支；可优化为函数指针表以降低维护成本。 |
| 风险 | 遗漏分支 → 空返回；缺乏默认日志会调试困难。 |

##### 11.1.5.A 现状痛点
- 可维护性差：新增类型需修改集中 switch，易遗漏；子类型（动画/变体）出现嵌套 switch。
- 不可扩展：外部插件无法无侵入添加新 Element 类型。
- 诊断弱：缺失分支时只得到空指针，无统一日志/统计。
- 枚举为 bitmask（`1ull<<n`），编译器难以构建紧凑跳转表（只是次要因素）。
- 代码臃肿：影响 Review 可读性和冲突率。

##### 11.1.5.B 目标
| 目标   | 量化/说明                           |
|------|---------------------------------|
| 动态扩展 | 插件模块在运行期注册新类型/子类型，无需改核心代码       |
| 去中心化 | 每个 Impl 自行注册其创建器（单一职责）          |
| 可诊断  | 未注册/重复注册输出统一日志；提供 Dump 接口       |
| 安全   | 不破坏现有调用 API；可宏控制回退旧实现           |
| 性能稳定 | O(1) 查找（哈希/数组），与当前 switch 相当或更好 |

##### 11.1.5.C 设计概览
采用“主类型注册 + 可选子类型注册”双层结构：
- PrimaryRegistry: key = 主类型 (uint64) → 记录一个默认创建器 + 子类型 map
- SubTypeCreators: key = 子类型值 (uint64) → 创建器
- 创建器签名：`TSharedPtr<IDatasmithElement> (*)(const TCHAR* Name, uint64 SubType)`
- 懒加载初始化：首次调用 CreateElement 时加载内置注册；插件在 StartupModule 中追加注册

##### 11.1.5.D 数据结构（示例）
```cpp
struct FDatasmithElementRegistry {
  using FCreateFunc = TSharedPtr<IDatasmithElement>(*)(const TCHAR*, uint64);
  struct FPrimaryEntry {
    FCreateFunc DefaultCreator = nullptr;              // 无子类型或默认 fallback
    TMap<uint64, FCreateFunc> SubTypeCreators;         // 子类型专用
  };
  static TMap<uint64, FPrimaryEntry>& Map();           // 单例存储
  static void Register(uint64 TypeKey, FCreateFunc Func, bool bOverride=false);
  static void RegisterSub(uint64 TypeKey, uint64 SubKey, FCreateFunc Func, bool bOverride=false);
  static TSharedPtr<IDatasmithElement> Create(EDatasmithElementType Type, uint64 SubType, const TCHAR* Name);
  static void InitBuiltins();                          // 集中初始化（懒加载）
};
```

##### 11.1.5.E 注册宏
```cpp
#define DATASMITH_REGISTER_ELEMENT(TypeEnum, Func) \
  namespace { struct FReg_##TypeEnum { \
    FReg_##TypeEnum(){ FDatasmithElementRegistry::Register((uint64)TypeEnum, Func);} \
  } GRegInst_##TypeEnum; }

#define DATASMITH_REGISTER_SUBTYPE(TypeEnum, Sub, Func) \
  namespace { struct FReg_##TypeEnum##_##Sub { \
    FReg_##TypeEnum##_##Sub(){ \
      FDatasmithElementRegistry::RegisterSub((uint64)TypeEnum, (uint64)Sub, Func);} \
  } GRegInst_##TypeEnum##_##Sub; }
```
> 可为插件提供可选 bOverride 标志以覆盖默认实现（调试/定制）

##### 11.1.5.F CreateElement 新实现骨架
```cpp
TSharedPtr<IDatasmithElement> FDatasmithSceneFactory::CreateElement(EDatasmithElementType T, const TCHAR* Name) {
  return CreateElement(T, 0, Name);
}

TSharedPtr<IDatasmithElement> FDatasmithSceneFactory::CreateElement(EDatasmithElementType T, uint64 Sub, const TCHAR* Name) {
  static bool bInit=false; if(!bInit){ FDatasmithElementRegistry::InitBuiltins(); bInit=true; }
  return FDatasmithElementRegistry::Create(T, Sub, Name);
}
```

##### 11.1.5.G 子类型策略
| 场景                         | 推荐方式                                                                                                   |
|----------------------------|--------------------------------------------------------------------------------------------------------|
| 材质表达式 (MaterialExpression) | 每种 `EDatasmithMaterialExpressionType` 独立注册（SUBTYPE）                                                    |
| 动画 (Animation)             | Transform / Visibility / Subsequence 分别注册子类型                                                           |
| Variant 树                  | LevelVariantSets / VariantSet / Variant / ActorBinding / PropertyCapture / ObjectPropertyCapture 作为子类型 |
| 低频类型（少量且稳定）                | 可以保持默认创建器内部小 switch，后续再拆                                                                               |

##### 11.1.5.H 诊断与错误处理
| 情况                     | 行为                               |
|------------------------|----------------------------------|
| 未注册主类型                 | 日志 Warning + 返回空指针               |
| 未注册子类型                 | 尝试 DefaultCreator → 若仍空则 Warning |
| 重复注册 (bOverride=false) | 保留首次，输出 Warning                  |
| 重复注册 (bOverride=true)  | 覆盖并 Info 日志                      |
| 非单一 bit 的 Type（组合）     | 警告并拒绝（未来可拓展 batch 创建）            |

##### 11.1.5.I 性能考虑
- 主类型数量 < 64，TMap 查表 O(1) 常数低；与目前 switch 分支预测差异微乎其微。
- 可在后期改为 `TSortedMap` + 二分或自定义开放寻址数组实现进一步微优化（必要性低）。
- 避免每次构造：使用静态单例+懒加载，不做锁；C++11 静态局部初始化线程安全。

##### 11.1.5.J 线程安全
| 阶段           | 策略                                     |
|--------------|----------------------------------------|
| 初始化          | 懒加载静态局部，C++11 保证一次性                    |
| 运行时注册（插件晚注册） | 要求在任何 CreateElement 前完成（模块 Startup 顺序） |
| 并发创建         | Map 只读（注册结束后不再写）；如需热插拔需加读写锁（暂不建议多源写）   |

##### 11.1.5.K 插件扩展示例
```cpp
// 插件 StartupModule()
extern TSharedPtr<IDatasmithElement> CreateMySpline(const TCHAR*, uint64);
DATASMITH_REGISTER_ELEMENT(EDatasmithElementType::CustomActor, &CreateMySpline);

// 若自定义子类型：
DATASMITH_REGISTER_SUBTYPE(EDatasmithElementType::MaterialExpression, (uint64)EDatasmithMaterialExpressionType::Custom, &CreateCustomExpr);
```

##### 11.1.5.L 回滚/双轨策略
| 宏                                  | 描述                 |
|------------------------------------|--------------------|
| `DATASMITH_FACTORY_USE_REGISTRY=0` | 默认：仍使用旧 switch（安全） |
| `DATASMITH_FACTORY_USE_REGISTRY=1` | 启用注册表实现            |
旧 switch 代码暂时保留，便于问题定位；达到覆盖率目标后再清除。

##### 11.1.5.M 迁移步骤（推荐）
1. 引入注册表基础结构 + 宏，不改原逻辑。
2. 迁移少数类型（Mesh / Light / Texture）验证日志 & 性能。
3. 迁移子类型密集的 MaterialExpression / Animation。
4. 添加自动审计脚本（扫描枚举 vs 注册表 key）。
5. 开启 CI 配置 `DATASMITH_FACTORY_USE_REGISTRY=1` 跑全部回归。
6. 稳定后删除旧 switch 或保留到下一个次要版本。

##### 11.1.5.N 自动审计脚本思路（伪 Python）
```python
# parse DatasmithDefinitions.h to collect EDatasmithElementType bit names
# parse registry cpp (or log dump) to collect registered type keys
missing = types - registered
duplicated = [k for k in registered if registered.count(k)>1]
```
> 可扩展到子类型：对比 `EDatasmithMaterialExpressionType` / `EDatasmithElementAnimationSubType` / `EDatasmithElementVariantSubType`。

##### 11.1.5.O 风险与规避总结
| 风险       | 规避                                              |
|----------|-------------------------------------------------|
| 注册顺序不确定  | 懒加载 + 内置集中初始化函数                                 |
| 遗漏注册     | 审计脚本 + 启动时 DumpRegistry + 单元测试“每类型至少可创建”        |
| 重复覆盖     | 统一日志提示，默认不覆盖                                    |
| 子类型散乱    | 规则：统一用枚举值数值做 SubKey；命名宏一致                       |
| 插件晚于首次调用 | 文档要求：插件 Startup 注册；或提供 ForceRebuild API（不建议多源写） |
| 调试不便     | `DumpRegistry()` 输出类型/子类型统计                     |

### 11.2 Private 目录逐文件解析

| 文件 / 组                       | 职责概述           | 关键数据结构 / 算法                | 扩展点                | 主要风险           | 快速验证脚本/用例          | 优化方向              |
|------------------------------|----------------|----------------------------|--------------------|----------------|--------------------|-------------------|
| *SceneElementsImpl*          | 基础元素实现/Actor 树 | Parent/Child 关系 + MD5 Hash | 新 Actor/字段         | Hash 漏字段/循环引用  | 构建→Hash→修改→Diff    | 名称→元素索引缓存         |
| *MaterialElementsImpl*       | 材质节点 Impl      | DFS 拓扑+参数序列 Hash           | 新节点类型              | 图循环/Hash 不稳定   | GraphHashStability | 子图缓存降低重复计算        |
| *AnimationElementsImpl*      | 动画帧管理          | 有序插入+去重                    | 新帧类型               | 大量帧退化          | 50k 帧插入基准          | 压缩/量化             |
| *VariantElementsImpl*        | Variant 树 Impl | 多层数组+属性捕获                  | 新 Capture 类型       | 路径变更不兼容        | 路径 Patch 测试        | 索引 Map            |
| Mesh.cpp                     | 网格数据操作         | Tangent/Bounds             | 新语义属性              | Index 越界       | 校验器脚本              | 并行切线              |
| MeshSerialization.cpp        | .geom I/O      | Mesh.h                     | Importer           | 版本不兼容          | 旧→新 RoundTrip      | Header schema 文档化 |
| MeshUObject.cpp              | 转 UStaticMesh  | Mesh.h / UE RHI            | Unreal Runtime     | Lightmap UV 错误 | 生成后可视检查            | 延迟构建 / 资源复用       |
| AnimationSerializer.cpp      | 动画二进制          | AnimationInterfaces        | Import/Export      | 版本回放失败         | 版本矩阵播放             | 魔数+版本校验           |
| Utils.cpp                    | 通用工具           | 路径/哈希/遍历                   | 新清理选项              | O(N^2) 遍历      | 大场景性能基准            | 引入缓存 & Lazy 索引    |
| MaterialsUtils.cpp           | 材质辅助           | 颜色转换/节点构造                  | 新色域                | 精度误差           | ΔE 测试              | LUT/缓存加速          |
| DirectLinkTools.cpp          | 调试工具           | Interfaces                 | 增量调试 / QA          | 大场景遍历慢         | 统计耗时               | 分页/过滤输出           |
| SceneReceiver.cpp            | 增量应用           | Interfaces / Factory       | DirectLink Runtime | 幂等 / 回滚失败      | 乱序+重复消息回放          | 属性位掩码节流           |
| LocaleScope.cpp              | Locale RAII    | setlocale/恢复               | 新模式                | 未恢复            | 异常路径测试             | 尝试线程局部化替代全局       |
| Core.cpp                     | 模块入口           | 日志/初始化                     | 注册表模式              | 初始化顺序          | 双重初始化测试            | 幂等性               |
| DatasmithLocaleScope.{h,cpp} | 区域设置 RAII      | setlocale/恢复               | 新模式                | 未恢复            | 异常路径测试             | 尝试线程局部化替代全局       |
| DirectLinkSceneReceiver.cpp  | 增量应用           | 批排序+属性更新                   | 新消息类型              | 不幂等            | 乱序+重复消息测试          | 位掩码 Diff          |
| Cloth* (Deprecated)          | 旧代码占位          | -                          | 移除                 | 遗留引用           | 枚举引用检查             | 标注移除计划            |

## 11.3 交叉引用矩阵（简化 → 增强版）
| 分组 / 文件(示例)               | 提供 (API/Impl)      | 直接依赖                                        | 主要被谁使用                       | 变更高风险点                | 首要测试聚焦                  | 备注 / 减耦建议         |
|---------------------------|--------------------|---------------------------------------------|------------------------------|-----------------------|-------------------------|-------------------|
| Definitions.h             | 枚举/常量              | CoreTypes                                   | 全部                           | 新枚举遗漏工厂/XML           | 枚举覆盖脚本                  | 考虑生成枚举→表驱动        |
| IDatasmithSceneElements.h | 公共接口               | Definitions / Types                         | Factory / Xml / Impl / Utils | 接口顺序 (ABI) 破坏         | 旧二进制加载                  | 拆分为多头文件可降低冲突      |
| SceneFactory.{h,cpp}      | 工厂 API+switch      | Interfaces / Material / Animation / Variant | Translators / XmlReader      | 漏 switch / 注册顺序       | 全类型创建循环                 | 迁移注册表后降低编辑冲突      |
| *SceneElementsImpl*       | Actor/Scene 等 Impl | Interfaces / Utils                          | Factory / Xml / DirectLink   | Hash 漏字段 / Actor 树父指针 | HashDiff / AttachDetach | 建索引加速名称查找         |
| *MaterialElementsImpl*    | 材质节点 Impl          | MaterialInterfaces                          | Factory / Xml                | 图循环 / Hash 不稳定        | GraphHashStability      | 子图缓存降低重复计算        |
| *AnimationElementsImpl*   | 动画帧 Impl           | AnimationInterfaces                         | Factory / Serializer / Xml   | 帧排序逻辑破坏               | 乱序插入后序列正确               | 可考虑稀疏帧优化          |
| *VariantElementsImpl*     | Variant 树 Impl     | VariantInterfaces                           | Factory / Xml                | 路径兼容 / 捕获值类型扩展        | VariantRoundTrip        | 路径 schema 需版本化    |
| Mesh*.h/.cpp              | Mesh 结构与工具         | CoreTypes                                   | Importer / MeshUObject       | 顶点/索引越界               | 大文件校验                   | 并行构建/流式加载         |
| MeshSerialization.cpp     | .geom I/O          | Mesh.h                                      | Importer                     | 版本不兼容                 | 旧→新 RoundTrip           | Header schema 文档化 |
| MeshUObject.cpp           | 转 UStaticMesh      | Mesh.h / UE RHI                             | Unreal Runtime               | Lightmap UV 错误        | 生成后可视检查                 | 延迟构建 / 资源复用       |
| XmlReader/Writer.cpp      | XML I/O            | Factory / Interfaces / Locale               | Import/Export                | PatchUpVersion 漏      | RoundTrip Diff          | SAX 资源并行解析（候选）    |
| AnimationSerializer.cpp   | 动画二进制              | AnimationInterfaces                         | Import/Export                | 版本回放失败                | 版本矩阵播放                  | 魔数+版本校验           |
| Utils.cpp                 | 通用遍历/清理            | Interfaces                                  | Translators / Tools          | O(N^2) 遍历             | 大场景性能基准                 | 引入缓存 & Lazy 索引    |
| MaterialsUtils.cpp        | 材质辅助               | Interfaces                                  | Translators / MaterialImpl   | 颜色精度丢失                | ΔE ≤ 阈测试                | LUT/缓存加速          |
| DirectLinkTools.cpp       | 调试工具               | Interfaces                                  | 增量调试 / QA                    | 大场景遍历慢                | 统计耗时                    | 分页/过滤输出           |
| SceneReceiver.cpp         | 增量应用               | Interfaces / Factory                        | DirectLink Runtime           | 幂等 / 回滚失败             | 乱序+重复消息回放               | 属性位掩码节流           |
| LocaleScope.cpp           | Locale RAII        | C runtime                                   | Xml I/O                      | 未恢复导致全局污染             | 嵌套异常测试                  | 线程局部化（待评估）        |
| SceneGraphSharedState.h   | 共享状态               | Core / STL                                  | Impl / DirectLink            | 粗粒度锁阻塞                | 并发压力                    | 分区锁/原子结构          |
| Cloth*(Dep)               | 占位                 | -                                           | -                            | 遗留引用                  | 枚举引用检查                  | 标注移除计划            |

---
## 12. 符号速查与接口清单（完整索引）
> 目的：为开发 / Code Review / 审计脚本提供“按类别快速定位”入口。覆盖接口、实现类、核心枚举、常量宏、关键函数签名与其所在文件。该清单只聚焦结构与职责，不重复 13~23 章深入分析。

### 12.1 类别索引概览
| 类别         | 子类目                                                                                         | 查找关键字建议                            | 主要文件                           | 参考章节        |
|------------|---------------------------------------------------------------------------------------------|------------------------------------|--------------------------------|-------------|
| Element 接口 | Scene / Actor / Mesh / Light / Camera / Material / Texture / Variant / Animation / MetaData | `class IDatasmith`                 | IDatasmithSceneElements.h      | 3 / 11      |
| Impl 实现    | *SceneElementsImpl / *MaterialElementsImpl / *AnimationElementsImpl / *VariantElementsImpl  | `FDatasmith.*Impl`                 | 对应 *Impl.h/.cpp                | 4 / 11      |
| 工厂         | FDatasmithSceneFactory                                                                      | `Create` 前缀                        | DatasmithSceneFactory.*        | 3 / 11 / 15 |
| 材质表达式      | Scalar/Color/Texture/Generic/FunctionCall/Flatten/Custom/Fresnel(若有)                        | `EDatasmithMaterialExpressionType` | DatasmithMaterialElements.*    | 3 / 15      |
| 动画         | Transform/Visibility/Subsequence/LevelSequence                                              | `IDatasmithTransformAnimation` 等   | DatasmithAnimationElements.*   | 3 / 11 / 15 |
| 变体         | LevelVariantSets/VariantSet/Variant/ActorBinding/PropertyCapture                            | Variant 关键字                        | DatasmithVariantElements.*     | 3 / 11 / 15 |
| Mesh & 序列化 | Mesh 数据/序列化/UObject 适配                                                                      | `FDatasmithMesh`                   | DatasmithMesh*.h/.cpp          | 3 / 11 / 15 |
| XML        | Reader / Writer / Patch                                                                     | `XmlReader` `XmlWriter`            | DatasmithSceneXml*             | 3 / 11 / 13 |
| 动画序列化      | Serializer                                                                                  | `AnimationSerializer`              | DatasmithAnimationSerializer.* | 3 / 11 / 16 |
| DirectLink | SceneReceiver / DirectLinkTools                                                             | `SceneReceiver`                    | DirectLink/*.h/.cpp            | 5 / 14 / 23 |
| 工具与杂项      | Utils / MaterialsUtils / LocaleScope                                                        | `DatasmithUtils`                   | 对应文件                           | 3 / 11 / 22 |

### 12.2 Element 接口核心方法速览（节选）
| 接口                                  | 关键方法                                                         | 说明      | 影响 Hash    | DirectLink 典型掩码            |
|-------------------------------------|--------------------------------------------------------------|---------|------------|----------------------------|
| IDatasmithElement                   | GetName / GetLabel / SetLabel / GetHash                      | 基础标识    | GetHash 结果 | -                          |
| IDatasmithActorElement              | Get/SetTransform / GetChildrenCount / GetChild(i) / AddChild | 层级与变换   | Transform  | Transform 位                |
| IDatasmithMeshElement               | GetFile / SetFile / GetMaterialSlotCount                     | 资源源文件与槽 | File(哈希)   | MeshRef 位                  |
| IDatasmithMeshActorElement          | SetStaticMeshPathName / AddMaterialOverride                  | 实例绑定与覆盖 | 覆盖列表       | MeshRef / MaterialOverride |
| IDatasmithLightActorElement         | SetIntensity/Color/Temperature/IES                           | 光照参数    | 强度/光色/IES  | LightCore / LightShape     |
| IDatasmithCameraActorElement        | SetFocalLength / SetSensor*                                  | 成像参数    | 视觉外观       | Camera(若存在)                |
| IDatasmithUEPbrMaterialElement      | AddMaterialExpression / GetXInput                            | 构建材质图   | 节点序列+参数    | MaterialGraph              |
| IDatasmithTransformAnimationElement | AddFrame / GetFramesCount                                    | 动画帧     | 帧序列        | Animation                  |
| IDatasmithLevelSequenceElement      | AddAnimation / GetAnimationsCount                            | 动画集合    | 子动画 Hash   | Animation                  |
| IDatasmithVariantElement            | AddActorBinding                                              | 变体组织    | 捕获集合       | VariantBinding             |
| IDatasmithPropertyCapture           | GetCategory / Serialize                                      | 属性捕获    | 值          | VariantBinding             |

### 12.3 Impl 实现（模板化派生）常见约定
| 模式            | 内容                             | 风险           | 审计点                      |
|---------------|--------------------------------|--------------|--------------------------|
| 构造登记          | 构造函数中 TReflectedParam.Register | 遗漏登记字段       | 见 13 章审计脚本               |
| MarkHashDirty | setter 改值后调用                   | 漏调用导致增量不刷新   | grep `MarkHashDirty` 覆盖率 |
| 子对象容器         | TArray / TSet                  | 顺序不稳定影响 Hash | 序列前排序/拓扑排序               |
| Hash 实现       | 遍历已登记字段或手写序列                   | 手写漏字段        | diffs vs 登记集             |

### 12.4 工厂创建函数分类
| 函数前缀                     | 示例                                | 返回           | 说明          | 关联枚举                         |
|--------------------------|-----------------------------------|--------------|-------------|------------------------------|
| CreateScene              | CreateScene                       | Scene 接口     | 场景根         | EDatasmithElementType::Scene |
| CreateMesh               | CreateMesh                        | Mesh 接口      | 网格资源        | Mesh bit                     |
| CreateMeshActor          | CreateMeshActor                   | MeshActor 接口 | 实例          | MeshActor bit                |
| CreateUEPbrMaterial      | CreateUEPbrMaterial               | PBR 材质接口     | UEPBR 管线    | Material bits                |
| CreateMaterialExpression | CreateMaterialExpression(SubType) | 表达式基类        | 分支于 SubType | MaterialExpression 枚举        |
| CreateTransformAnimation | CreateTransformAnimation          | Transform 动画 | 曲线集合        | Animation SubType            |

### 12.5 关键枚举速览
| 枚举                                | 目的      | 典型值示例                                            | 注意           |
|-----------------------------------|---------|--------------------------------------------------|--------------|
| EDatasmithElementType             | 元素大类（位） | Scene / Mesh / MeshActor / Light / VariantSet    | 位不重复；扩展需更新工厂 |
| EDatasmithElementAnimationSubType | 动画子类    | Transform / Visibility / Subsequence             | Hash 分类      |
| EDatasmithElementVariantSubType   | 变体子类    | LevelVariantSets / VariantSet / Variant          | 构建层级         |
| EDatasmithMaterialExpressionType  | 材质节点类型  | Scalar / Texture / Color / Custom / FunctionCall | 添加需同步解析/Hash |
| EDatasmithTransformChannels       | 动画通道位   | Translation/Rotation/Scale/Visibility            | 位掩码稳定        |
| EDatasmithPropertyCategory        | 变体属性分类  | Transform / Material / Exposure(扩展)              | Patch 兼容     |

### 12.6 常量宏（用途分组）
| 分组         | 前缀/示例                            | 用途     |
|------------|----------------------------------|--------|
| XML 节点/属性名 | DATASMITH_*NAME                  | 读写对称键  |
| Light 参数   | DATASMITH_LIGHT*                 | 光照字段统一 | 
| 材质/纹理      | DATASMITH_MATERIAL* / *_TEXTURE* | 通道/采样键 | 
| 动画         | DATASMITH_ANIM*                  | 序列化标签  | 
| 变体         | DATASMITH_VARIANT*               | 变体结构   | 

### 12.7 Hash / 增量相关重点符号
| 符号                           | 类型  | 作用              | 章节      |
|------------------------------|-----|-----------------|---------|
| CalculateElementHashInternal | 虚函数 | 元素级 Hash        | 13 / 14 |
| MarkHashDirty                | 方法  | 标记需重算           | 13 / 14 |
| Layered Hash (L0/L1)         | 概念  | 分层差异            | 13 / 14 |
| Property Bit Mask            | 位掩码 | DirectLink 粒度控制 | 14 / 23 |

### 12.8 XML 读写辅助
| 符号                      | 类别    | 说明             |
|-------------------------|-------|----------------|
| DatasmithSceneXmlReader | 类     | 构建元素骨架 & Patch | 
| DatasmithSceneXmlWriter | 类     | 顺序/省略默认输出      | 
| PatchUpVersion          | 函数/方法 | 版本迁移           | 
| LocaleScope             | RAII  | 小数点/区域一致       | 

### 12.9 动画与变体核心符号
| 范畴              | 符号                                   | 说明       |
|-----------------|--------------------------------------|----------|
| Transform 动画    | IDatasmithTransformAnimationElement  | 帧序列 + 通道 |
| Visibility 动画   | IDatasmithVisibilityAnimationElement | 帧显隐      |
| LevelSequence   | IDatasmithLevelSequenceElement       | 动画集合     |
| Variant 根       | IDatasmithLevelVariantSets           | 变体集合根    |
| PropertyCapture | IDatasmithBasePropertyCapture        | 通用属性快照   |

### 12.10 DirectLink 关键符号
| 符号                     | 说明                    | 用途          |
|------------------------|-----------------------|-------------|
| DatasmithSceneReceiver | 接收并应用增量               | 协议终端        |
| GetElementTypeName     | 类型名映射                 | 调试/日志       |
| BulkHashSync (概念)      | 全量校准消息                | Hash 漏检纠偏   |
| 位掩码常量（建议）              | Transform/LightCore 等 | Update 合并策略 |

### 12.11 常用工具/助手
| 符号                             | 文件              | 说明         |
|--------------------------------|-----------------|------------|
| NormalizePath / MakeUniqueName | DatasmithUtils  | 路径/命名处理    |
| BuildTangents (若存在)            | Mesh.cpp        | 网格切线重建     |
| QuantizeFloat (策略)             | Utils/Hash      | 跨平台稳定 Hash |
| DumpDatasmithScene             | DirectLinkTools | 场景概要调试     |

### 12.12 审计脚本目标对照
| 目标        | 脚本                    | 关键数据源                 | 失败指示                |
|-----------|-----------------------|-----------------------|---------------------|
| 枚举覆盖      | audit_factory_enum.py | Definitions + Factory | 缺失枚举 case           |
| Hash 字段   | audit_hash_fields.py  | Impl 登记 vs Hash 实现    | 漏/多字段列表             |
| 版本标记      | audit_versions.py     | git diff + Impl       | VersionIntroduced=0 |
| Legacy 清理 | audit_legacy_usage.py | LegacyNames + Patch   | 未迁移旧名               |
| XML 对称    | xml_roundtrip_diff    | 导出+再导出 Diff           | 非时间戳差异              |

### 12.13 快速搜索提示
| 需求            | VSCode / grep 示例                         | 说明     |
|---------------|------------------------------------------|--------|
| 找所有 Impl 登记   | `grep -R "Register("`                    | 统计字段   |
| 定位 Hash 函数    | `grep -R "CalculateElementHashInternal"` | 检查覆盖   |
| 枚举差异          | `git diff -U0 -- Definitions.h`          | 枚举变更点  |
| 反射字段 vs Hash  | `grep -R "TReflectedParam"` + Hash 函数    | 差集审计   |
| DirectLink 应用 | `grep -R "ApplyUpdate"`                  | 更新路径跟踪 |

### 12.14 风险提示总览（与其它章交叉）
| 风险            | 触发        | 影响         | 相关章          | 缓解             |
|---------------|-----------|------------|--------------|----------------|
| Hash 漏字段      | 新增未登记或未序列 | 增量失效/缓存不刷  | 13 / 14 / 17 | 审计脚本 Gate      |
| 枚举未覆盖工厂       | 新增枚举      | Create 返回空 | 11 / 15 / 17 | 枚举覆盖测试         |
| Patch 迁移遗漏    | 旧 XML     | 旧键残留/值丢    | 13 / 22      | 旧文件回归集         |
| DirectLink 乱序 | 网络延迟      | 场景不一致      | 14 / 23      | 序列校验+重放测试      |
| XML 省略过度      | 默认值逻辑错误   | 还原缺属性      | 13 / 22      | RoundTrip Diff |

### 12.15 使用建议
- 双向导航：章节 12 给“在哪里”，章节 13+ 给“怎么做 / 为什么”。
- 审计自动化优先覆盖：枚举、Hash、版本迁移、XML RoundTrip。
- 新增类型：先更新索引表（12.1/12.4/12.5），再补实现，最后跑审计脚本。

---
## 13. 实现层反射 / 参数存储机制深入解析（增强长版）
> 目的：系统阐述 Impl 层“轻量反射(Reflective Param Registry)”思想、数据结构、调用序列、Hash / XML / DirectLink 复用路径、版本迁移策略、性能/正确性平衡与自动化审计体系。

### 13.1 背景与动机
| 问题              | 传统做法痛点             | 轻量反射带来收益                             |
|-----------------|--------------------|--------------------------------------|
| Hash 漏字段        | 手写 Update，新增字段忘记添加 | 可自动枚举 AffectsHash 字段，减少遗漏            |
| XML 写入冗余        | 无默认值信息，全部硬编码输出     | DefaultValue 元数据→省略默认值               |
| XML 兼容迁移        | 旧字段改名后多处 if/else   | LegacyNames / VersionIntroduced 集中映射 |
| DirectLink Diff | 按元素结构手工比较          | 遍历登记字段生成 差异列表/位掩码                    |
| 可测试性差           | 需人工回忆字段列表          | 审计脚本比对“登记集合 vs Hash/Writer 使用集合”     |
| 性能不稳            | 大量散布 if/strcmp     | 顺序数组线性遍历 + bitflag 筛选可预测             |

### 13.2 设计目标
| 目标    | 说明                     | 辅助措施                                            |
|-------|------------------------|-------------------------------------------------|
| 低侵入   | 不引入重量 RTTI / 反射系统      | 模板 + 扁平数组而非复杂类型树                                |
| 确定顺序  | 多平台 Hash 一致            | 插入顺序索引数组稳定化                                     |
| 条件筛选  | Hash / XML / Diff 不同需求 | Flag 位 (AffectsHash / XmlPersisted / Transient) |
| 版本感知  | PatchUpVersion 精准处理    | 每字段 VersionIntroduced & LegacyNames[]           |
| 轻量量化  | 浮点 Hash 稳定             | QuantizePolicy (epsilon / 乘除)                   |
| 审计可拓展 | 外部脚本检视                 | 生成“字段登记清单导出”接口                                  |

### 13.3 核心数据结构（参考实现伪编码）
```cpp
enum class EParamFlag : uint8 {
  None=0, AffectsHash=1<<0, XmlPersisted=1<<1, Transient=1<<2, DiffImportant=1<<3
};
ENUM_CLASS_FLAGS(EParamFlag);

template<typename T>
struct TReflectedParam {
  const TCHAR* Name = nullptr;
  T             Value{};
  T             DefaultValue{};
  EParamFlag    Flags = EParamFlag::None;
  int16         VersionIntroduced = 0; // 0 = prehistory
  TArray<const TCHAR*> LegacyNames;    // 顺序：旧→更旧
  // 量化策略（可选）
  float         QuantStep = 0.f;       // 0 => no quant

  void Register(const TCHAR* InName, const T& Def, EParamFlag F, int16 Ver=0, float QS=0.f){
     Name=InName; Value=DefaultValue=Def; Flags=F; VersionIntroduced=Ver; QuantStep=QS;
     FParamRegistry::Get().Add(this);
  }
};

struct FParamEntryView { const void* Ptr; EParamFlag Flags; const TCHAR* Name; int16 Ver; /*type id...*/ };

class FParamRegistry {
  TArray<FParamEntryView> Entries; // 稳定顺序
public:
  template<typename P> void Add(P* Param){ Entries.Add({Param, Param->Flags, Param->Name, Param->VersionIntroduced}); }
  auto Begin() const { return Entries.begin(); }
  auto End() const { return Entries.end(); }
  static FParamRegistry& Get();
};
```

### 13.4 Hash 计算流程与稳定性策略
| 步骤 | 描述              | 细节                                |
|----|-----------------|-----------------------------------|
| 1  | 遍历注册表（顺序数组）     | 不依赖哈希表迭代随机性                       |
| 2  | 跳过非 AffectsHash | Flags & AffectsHash==0            |
| 3  | 量化浮点            | 若 QuantStep>0, v = round(v/qs)*qs |
| 4  | 写字段标签           | type + name hash + length         |
| 5  | 写字段值            | 原始字节或序列化/再量化                      | 
| 6  | 子结构             | 子对象嵌套调用（深度优先）                     |
| 7  | 缓存层             | (可选) param 值变更计数 -> 未变化跳过         |

> 字段标签建议包含：TypeId(1 byte) + NameHash(4) + ValueSize(可选可推导)；Name 字面不重复写以降低 Hash 数据体参与长度。

### 13.5 XML 写入策略
| 策略    | 说明                            | 好处      | 风险             | 缓解            |
|-------|-------------------------------|---------|----------------|---------------|
| 省略默认  | 值 == DefaultValue 跳过          | 减少体积    | RoundTrip 忽略差异 | 读侧补默认         |
| 顺序控   | 资源→材质→Mesh→Actor→Variant→Anim | Diff 稳定 | 架构变动需调整        | CONSTANT 列表集中 |
| 精度裁剪  | 浮点写入 `std::to_chars` + trim   | 减少噪声    | 极端高精度丢失        | 配置最大小数位       |
| 旧名吸收  | 读：若新名缺→查 LegacyNames          | 兼容性     | 多次迁移链冗长        | 迁移日志审计        |
| 数值合法化 | clamp 到约束范围                   | 避免异常值污染 | 信息丢失           | 写入警告日志        |

### 13.6 PatchUpVersion 详解
| 阶段 | 处理内容                    | 示例                             |
|----|-------------------------|--------------------------------|
| 收集 | 旧属性节点放入 map             | "IntensityLm" → "2000"         |
| 匹配 | 遍历已登记字段 LegacyNames     | 找到 "IntensityLm"               |
| 转换 | 调整单位 (Lumens->Candelas) | *0.01f                         |
| 回写 | 写入新字段 value             | Intensity=20                   |
| 记录 | 日志统计 + 计数               | Migrated:IntensityLm→Intensity |
| 清理 | 删除旧节点                   | 避免重复写出                         |

### 13.7 DirectLink 差异提取
| 步骤 | 描述                | 输出                                    |
|----|-------------------|---------------------------------------|
| A  | 遍历 AffectsHash 字段 | 旧值缓存 vs 新值现值                          |
| B  | 比较值               | (changed? set bit)                    |
| C  | 汇总位掩码             | TransformGroup / LightGroup...        |
| D  | 构建消息              | {id, mask, per-field compact payload} |
| E  | 合并                | 同元素多次更新合并 payload                     |

### 13.8 性能调优点
| 问题           | 现象         | 调优手段                     | 结果目标      |
|--------------|------------|--------------------------|-----------|
| 大场景 Hash 时间长 | >100ms 帧阻塞 | 分块线程池 + ParamChangeBit   | 降低 50%+   |
| 重复字段序列化      | XML 体积增长   | 省略默认/重复字面字符串池            | -10~25%   |
| 浮点抖动         | Diff 频繁    | 量化 & clamp               | Hash 波动消失 |
| 字段查找 O(n)    | 注册表大       | 二级索引 map<name, ptr>（仅工具） | 调试快       |

### 13.9 自动化审计脚本（策略）
| 名称                    | 功能               | 伪逻辑关键点                                                                       |
|-----------------------|------------------|------------------------------------------------------------------------------|
| audit_hash_fields.py  | 检测 Hash 遗漏       | parse Impl: collect TReflected→ parse CalculateElementHashInternal→ set diff |
| audit_versions.py     | 检测无版本标记新字段       | git diff 最近 N 天 + 无 VersionIntroduced                                        |
| audit_legacy_usage.py | 未使用的 LegacyNames | LegacyNames 列表 - PatchUpVersion 匹配集合                                         |
| audit_param_flags.py  | Flags 合理性        | 统计没有 XmlPersisted 却出现在 XML 写出的字段                                             |

### 13.10 常见陷阱清单
| 陷阱                | 根因                        | 影响           | 解决             |
|-------------------|---------------------------|--------------|----------------|
| 字段未 MarkHashDirty | setter 漏调用                | 增量不刷新        | 统一 setter 模板包裹 |
| LegacyName 链过长    | 多轮改名串联                    | 解析遍历成本       | 合并：旧→最新(一次跳)   |
| QuantStep 设过大     | 视觉可见跳变                    | 视觉 artifacts | 以领域误差阈值评估      |
| Flags 冲突          | 同时 Transient+XmlPersisted | 逻辑矛盾         | 静态断言校验         |

### 13.11 进阶：分层 Hash（Layered Hash）
| 层  | 内容                        | 更新触发    | 用途             |
|----|---------------------------|---------|----------------|
| L0 | 参数组（Transform, LightCore） | 对应字段变   | 生成属性位掩码        |
| L1 | 元素总体                      | 任一 L0 改 | DirectLink 粗比较 |
| L2 | 资源/子图                     | 资源子节点变  | 材质/纹理缓存复用      |
| L3 | 场景聚合                      | 大量元素合并  | 远端一次性校验        |

### 13.12 可选：字段影子缓存 (Shadow Cache)
| 目的     | 说明                   | 代价       |
|--------|----------------------|----------|
| 减少重复拷贝 | 原值 + 影子值对比无改动跳过 Hash | 内存增加 ~ 倍 |
| 快速回滚   | 增量失败时恢复旧值            | 管理复杂     |

### 13.13 与 UE 反射差异
| 维度     | UE UPROPERTY             | 轻量反射方案          |
|--------|--------------------------|-----------------|
| 运行时遍历  | 反射系统 RTTI 支撑             | 手工注册数组          |
| 元数据丰富度 | Category/EditCondition 等 | 定制最少元数据         | 
| 序列化    | FArchive / UHT 生成        | 自定义 XML/Hash 格式 |
| ABI 稳定 | 需保持 UCLASS 布局            | 仅内部 Impl，不对外暴露  |

---
## 14. DirectLink 与 Hash 协同工作流程（增强长版）
> 深入拆解增量同步中 Hash 标脏、差异提取、消息构建、网络批处理、接收端应用、刷新节流与一致性保障机制。

### 14.1 角色与边界
| 角色            | 职责              | 不做的事         |
|---------------|-----------------|--------------|
| Translator（源） | 构建/更新场景元素，标记脏   | 不关心接收端结构细节   |
| DeltaBuilder  | 对比旧新 Hash / 参数值 | 不处理网络发送      |
| Sender        | 批量封包发送          | 不解析业务语义      |
| Receiver      | 验证顺序、应用差异、重建引用  | 不重新生成 Diff   |
| View/Render   | 根据属性位掩码重建缓存     | 不维护 Hash 原始值 |

### 14.2 标脏机制
| 行为               | 触发         | 标记内容                               | 合并策略    |
|------------------|------------|------------------------------------|---------|
| SetTransform     | 外部调用       | Element.TransformDirty=true        | 同一帧多次合并 |
| SetIntensity     | Light 属性修改 | Element.LightCoreDirty=true        | 覆盖取末次   |
| OverrideMaterial | 覆盖表增删      | Element.MaterialOverrideDirty=true | 列表 diff |

### 14.3 差异提取（参数级）伪算法
```cpp
for each DirtyElement:
  // L0 Hash（参数组）以缓存的旧 Hash vs 新 Hash 判定
  RecalcGroupHash(Element, GroupMaskOut)
  if(GroupMaskOut != 0){
     BuildParamPayload(Element, GroupMaskOut)
     PendingMessages.Add(Update{ID, Mask, Payload})
  }
FlushCoalesce(PendingMessages)
```

### 14.4 消息结构建议 (二进制)
| 字段          | 长度     | 描述                             |
|-------------|--------|--------------------------------|
| Type        | 1B     | 0=Add,1=Upd,2=Rep,3=Rem,4=Bulk |
| ElementId   | VarInt | ID 压缩                          |
| Mask        | VarInt | 位掩码                            |
| FieldCount  | VarInt | 仅 Update                       |
| [Fields...] | 可变     | nameHash + value               | 
| Hash (可选)   | 8B     | 增量后新元素 Hash 截断                 |

### 14.5 批处理策略
| 策略                | 优点     | 风险    | 适用     |
|-------------------|--------|-------|--------|
| 固定时间片 (e.g. 50ms) | 控制延迟上界 | 空闲仍发送 | 交互编辑   |
| 固定数量上限 (N msgs)   | 减少包头开销 | 尾部等待  | 批量导入   |
| 自适应 (延迟 + 队列深度)   | 动态平衡   | 实现复杂  | 变动波动场景 |

### 14.6 幂等与顺序保障
| 机制                  | 描述             | 失败处理                |
|---------------------|----------------|---------------------|
| BatchSeq++          | 批次单调递增         | 丢包：若 seq 跳跃→请求重发或忽略 | 
| Per-Element Version | 更新附带局部版本号      | 版本回退 → 忽略较旧         |
| Stable Parent First | Add 资源后 Add 引用 | 乱序引用→延迟队列           |

### 14.7 回滚策略
| 场景           | 步骤              | 结果      |
|--------------|-----------------|---------|
| 部分 Update 失败 | 记录失败字段→不提交 Hash | Hash 仍旧 | 
| Reparent 循环  | 检测到环→取消该操作      | 树完整     |
| 资源缺失         | 标记占位 + 延迟解析     | 后续补全    |

### 14.8 渲染层刷新决策（示例）
| 位掩码              | 缓存动作                  |
|------------------|-----------------------|
| Transform        | 更新矩阵缓冲 / Bounding 结构  |
| LightCore        | 更新光强/颜色数据 UBO         |
| LightShape       | 重建几何代理                |
| MeshRef          | 请求 Streaming / 绑定 VBO |
| MaterialOverride | 重建材质实例参数 block        |

### 14.9 性能度量指标
| 指标                 | 目标        | 收集方式      |
|--------------------|-----------|-----------|
| Avg Batch Size     | 50~200 更新 | Sender 统计 |
| Hash Recalc Time   | P95 < 5ms | 计时聚合      |
| Update Msg Drop(%) | ~0        | 序号对账      |
| Redundant Updates  | <5%       | 比较旧新值相等计数 |

### 14.10 安全 / 可靠性
| 要素   | 说明     | 选项               |
|------|--------|------------------|
| 签名   | 防篡改    | HMAC(批 payload)  |
| 压缩   | 大批减少网络 | LZ4 Frame / Zstd |
| 速率限制 | 防洪泛滥   | TokenBucket      |
| 兼容   | 多版本协同  | FeatureBits 字段   |

### 14.11 测试用例要点
- 幂等性：重放前后 Hash 不变，状态一致。
- 顺序：消息乱序时，依赖关系不破坏，最终状态一致。
- 批处理：大批量消息处理不超时，内存不暴涨。

---
## 15. 扩展示例代码模板（增强长版）
> 汇集多类扩展场景模板（新 Actor / 新材质表达式 / 新 Variant Capture / 新动画帧 / DirectLink 消息 / 序列化扩展 / Hash 分层加入 / PatchUpVersion 增补）。

### 15.1 新 Actor 全流程骨架
```cpp
// 1) Definitions.h
enum class EDatasmithElementType : uint64 { /*...*/, TubeLightActor = 1ull<<38 };

// 2) 接口 IDatasmithTubeLightActorElement
class IDatasmithTubeLightActorElement : public IDatasmithLightActorElement {
 public:
  virtual float GetLength() const = 0;
  virtual void  SetLength(float) = 0;
  virtual float GetRadius() const = 0;
  virtual void  SetRadius(float) = 0;
};

// 3) Impl
class FDatasmithTubeLightActorImpl final : public FDatasmithLightActorElementImpl<IDatasmithTubeLightActorElement> {
  TReflectedParam<float> Length;  // AffectsHash
  TReflectedParam<float> Radius;  // AffectsHash
public:
  FDatasmithTubeLightActorImpl(const TCHAR* N):Super(N) {
    Length.Register(TEXT("Length"), 1.f,  EParamFlag::AffectsHash,  VERSION_2025_1, 0.001f);
    Radius.Register(TEXT("Radius"), 0.05f, EParamFlag::AffectsHash, VERSION_2025_1, 0.001f);
  }
  float GetLength() const override { return Length.Value; }
  void  SetLength(float V) override { if(Length.Value!=V){ Length.Value=V; MarkHashDirty(); }}
  float GetRadius() const override { return Radius.Value; }
  void  SetRadius(float V) override { if(Radius.Value!=V){ Radius.Value=V; MarkHashDirty(); }}
  void CalculateElementHashInternal(FMD5& MD5) const override { HashFloat(MD5, Length.Value); HashFloat(MD5, Radius.Value); }
};

// 4) 工厂 switch / 注册表
case EDatasmithElementType::TubeLightActor: return MakeShared<FDatasmithTubeLightActorImpl>(InName);

// 5) XML
// <TubeLightActor Name="L1" Length="2.0" Radius="0.1" Color="..." Intensity="..." />

// 6) PatchUpVersion (若旧版字段 TubeLength -> Length)
MigrateFloat(TEXT("TubeLength"), TEXT("Length"));
```

### 15.2 新材质表达式节点 (自定义噪声 Texture Generator)
| 步骤        | 文件                                         | 重点                        |
|-----------|--------------------------------------------|---------------------------|
| 枚举扩展      | EDatasmithMaterialExpressionType::NoiseTex | 位置与已有枚举不冲突                |
| 接口（可复用基类） | MaterialElements.h                         | 仅需构造实例 & 参数访问             |
| Impl      | MaterialElementsImpl                       | 哈希包含：Seed, Scale, Octaves |
| 工厂        | SceneFactory.cpp                           | SubType case              |
| XML       | Writer/Reader                              | 仅写入非默认参数                  |
| Hash      | DFS 节点拓扑                                   | 防循环                       |

### 15.3 Variant 新属性捕获（曝光 Exposure）
```cpp
enum class EDatasmithPropertyCategory : uint8 { /*...,*/ Exposure=42 };
class FDatasmithExposurePropertyCaptureImpl : public FDatasmithBasePropertyCaptureImpl {
  TReflectedParam<float> Exposure;
public:
  FDatasmithExposurePropertyCaptureImpl(){ Exposure.Register(TEXT("Exposure"),1.0f,EParamFlag::AffectsHash); }
  virtual EDatasmithPropertyCategory GetCategory() const override { return EDatasmithPropertyCategory::Exposure; }
  virtual void SerializeToXml(FXmlNode& Node) const override { if(Exposure.Value!=Exposure.DefaultValue) Node.SetFloat("Exposure", Exposure.Value);}
};
```

### 15.4 新动画帧类型（Morph Weight）
| 元素         | 内容                                          |
|------------|---------------------------------------------|
| 枚举         | EDatasmithTransformType::Morph              |
| 帧结构        | FDatasmithMorphFrame{ FrameNumber, Weight } |
| AddFrame   | 插入按 FrameNumber 排序，重复 Frame 合并              |
| Hash       | 时间与量化 Weight 值序列                            |
| Serializer | 新 block ID + 帧数 + (Frame,Value)             |

### 15.5 DirectLink 新消息：BulkMaterialParamUpdate
| 字段                 | 描述       |
|--------------------|----------|
| MaterialId         | 目标材质     |
| Count              | 参数个数     |
| [NameHash,Value]×N | 参数修改     |
| NewMaterialHash    | 可选用于快速校验 |

应用逻辑：
1. 找材质 Impl -> 更新参数 (MarkHashDirty)；
2. 若部分参数无效→记录失败列表→局部回滚；
3. 成功后触发材质实例缓存重建。

### 15.6 Mesh 序列化扩展新增压缩标志
| 变更                              | 行为                                 |
|---------------------------------|------------------------------------|
| Header 增加 Flags bit: COMPRESSED | 若置位 -> 数据块采用 LZ4 (块头: 原大小 + 压缩后大小) |
| Version++                       | PatchUpVersion：旧版本默认无压缩            |
| Writer                          | 可配置策略：顶点数 > X 启用                   |
| Reader                          | 检查标志 -> LZ4 解压 -> 验证数据量匹配          |

### 15.7 Hash 分层加入（示例）
```cpp
struct FElementHashes { uint64 GroupHash[GroupCount]; uint64 ElementHash; }
// 更新：先计算受影响组 GroupHash[i]，再聚合：ElementHash = CityHash64(concat(GroupHash[i]))
```

### 15.8 PatchUpVersion 增补模板
```cpp
void FMyScenePatch::PatchUpVersion(int FileVer){
  if(FileVer < VER_2025_LIGHT_RENAME){ MigrateFloat(TEXT("IntensityLm"), TEXT("Intensity"), ConvertLmToCd); }
  if(FileVer < VER_2025_EXPOSURE_ADD){ if(!HasAttribute("Exposure")) SetAttribute("Exposure", "1.0"); }
}
```

### 15.9 单元 / 集成测试建议
| 维度            | 用例                  | 断言         |
|---------------|---------------------|------------|
| Hash 稳定       | 同场景两次计算             | 值相等        |
| 漏字段审计         | 人为移除一字段 Hash Update | 脚本报警       |
| Patch 兼容      | 旧 XML 导入            | 新字段全部存在    |
| DirectLink 幂等 | 重放同批                | 场景 Hash 不变 |
| 性能回归          | >5% 基线              | Review     |

---
## 16. 回归测试矩阵（增强长版）
> 组织通过“域 × 类型 × 规模 × 平台”正交组合与重点场景补点覆盖。

### 16.1 域/类型枚举
| 域           | 描述              |
|-------------|-----------------|
| Mesh        | 网格结构/序列化/导入     |
| Material    | 材质图/表达式/Hash    |
| Animation   | 帧序列/序列化/播放      |
| Variant     | 属性捕获/切换         |
| DirectLink  | 增量消息/顺序/幂等      |
| XML         | 读写/Patch/Locale |
| Performance | 基准/内存/并行        |
| Locale      | 区域设定差异          |

| 类型            | 描述           |
|---------------|--------------|
| RoundTrip     | 导出→导入一致性     |
| HashDiff      | 单字段变动 Hash 变 |
| Compatibility | 旧文件迁移        |
| Stress        | 大规模/高频       |
| Negative      | 非法输入/边界      |
| Performance   | 耗时/内存统计      |

### 16.2 覆盖表样例
| Case         | 域           | 类型            | 规模    | 平台  | 描述                    | 关键断言         |
|--------------|-------------|---------------|-------|-----|-----------------------|--------------|
| MESH-RT-01   | Mesh        | RoundTrip     | Small | Win | 单三角网格往返               | 顶点/索引相等      |
| MESH-RT-02   | Mesh        | RoundTrip     | Large | Win | 50 万三角 LOD            | LOD 数/顶点数一致  |
| MAT-HASH-01  | Material    | HashDiff      | Small | Win | 改 Scalar 节点           | Hash 有变且仅材质变 |
| MAT-PATCH-02 | Material    | Compatibility | Small | Win | 旧字段 Rough -> Specular | Patch 后新字段存在 |
| ANIM-NEG-01  | Animation   | Negative      | Small | All | 乱序插入帧                 | 帧序升序且无重复     |
| VAR-RT-01    | Variant     | RoundTrip     | Small | All | 简单捕获                  | 捕获数量一致       |
| DL-STRESS-01 | DirectLink  | Stress        | Large | Win | 10k Update 批          | 幂等 重放=同 Hash |
| XML-LOC-01   | XML         | Compatibility | Small | Mac | 法/德语 Locale           | 数值解析正确       |
| PERF-HASH-01 | Performance | Performance   | Large | Win | 2k 材质 Hash            | P95<阈值       |

### 16.3 指标收集表
| 指标                 | 目标        | 收集方式      |
|--------------------|-----------|-----------|
| Avg Batch Size     | 50~200 更新 | Sender 统计 |
| Hash Recalc Time   | P95 < 5ms | 计时聚合      |
| Update Msg Drop(%) | ~0        | 序号对账      |
| Redundant Updates  | <5%       | 比较旧新值相等计数 |

### 16.4 Flaky 管理
| 策略   | 描述                     |
|------|------------------------|
| 重试阈值 | 单测失败自动重跑 1 次           |
| 标记   | 连续 3 天出现 → 标记 flaky 列表 |
| 优先修复 | Blocker 优先级降级规则        |

---
## 17. 变更影响映射（增强长版）
> 将“变更类型”映射到“需要触发的测试/审计/人工检视”，实现最小充分回归。

### 17.1 变更类型细化
| 代号            | 描述                    | 示例                        |
|---------------|-----------------------|---------------------------|
| C_ENUM        | 新增/修改枚举               | 新 LightType               |
| C_IFACE       | 接口签名/方法变更             | 新 API: GetShadowBias      |
| C_IMPL_FIELD  | Impl 新增参数             | 新字段 bTwoSided             |
| C_HASH        | Hash 算法/字段序列变更        | 引入分层 Hash                 |
| C_XML_SCHEMA  | XML 标签/属性结构更改         | <Length> -> attr="Length" |
| C_DL_PROTO    | DirectLink 协议新增消息     | BulkMaterialParamUpdate   |
| C_SER_FMT     | 二进制格式 header/body 更新  | .geom 添加压缩标记              |
| C_PERF_CRIT   | 性能敏感段重写               | 并行 Hash 重构                |
| C_PATCH_LOGIC | PatchUpVersion 迁移逻辑修改 | Intensity 旧换算             |

### 17.2 映射矩阵
| 变更            | 必跑测试                                | 审计脚本                   | 人工 Review 要点       |
|---------------|-------------------------------------|------------------------|--------------------|
| C_ENUM        | FactoryCoverage / RoundTrip         | audit_factory_enum.py  | 枚举值冲突/文档更新         |
| C_IMPL_FIELD  | HashDiff / Variant/DirectLink Basic | audit_hash_fields.py   | Default / Flags 合理 |
| C_HASH        | HashStability / Replay              | hash_replay_compare.py | 白名单更新 & 误差评估       |
| C_XML_SCHEMA  | XmlRoundTrip / Compat / Locale      | xml_roundtrip_diff     | PatchUpVersion 完整  |
| C_DL_PROTO    | DirectLinkStress / Replay           | dl_schema_dump.py      | 幂等设计 & 顺序保障        |
| C_SER_FMT     | BinaryCompat / VersionUpgrade       | bin_header_scan.py     | 版本号 bump / 文档      |
| C_PERF_CRIT   | PerfBaseline                        | perf_scan_scene        | 退化报警阈值设定           |
| C_PATCH_LOGIC | LegacySampleRoundTrip               | patch_map_audit.py     | 旧→新映射完整            |

### 17.3 流程
1. PR 标注变更类型标签 (可多选)。
2. CI 动态选择测试集合 + 审计脚本。
3. 脚本失败 → 阻断；测试失败 → 阻断。
4. 人工评审：核查文档/版本/白名单。
5. 合入后生成变更报告（枚举 diff、Hash 白名单 diff、协议 diff）。

---
## 18. 未来演进路线（增强长版）
| 主题              | 优先级 | 描述                     | 成功度量                | 里程碑                   |
|-----------------|-----|------------------------|---------------------|-----------------------|
| 工厂注册表化          | P0  | 替换大 switch             | 代码冲突率下降 / 插件可扩展     | Beta→默认启用             |
| Hash 分层         | P1  | L0/L1/L2 分层            | 增量消息平均 payload -30% | Prototype / Benchmark |
| 子图 Hash 缓存      | P1  | 材质/合成贴图子图缓存            | 大型材质 Hash 时间 -50%   | 命中率>80%               |
| XML 流式解析        | P1  | SAX + 并行资源             | 导入时间 -15%           | 与旧生成 Diff=0           |
| 差异位掩码压缩         | P2  | RLE/BitPack            | 网络带宽减少              | 消息体 -10%              |
| DirectLink 增量压缩 | P2  | Zstd +/-DeltaPack      | 批平均大小 -40%          | 端到端延迟不增               |
| 场景增量签名          | P2  | HMAC 批签名               | 篡改检测率 100%          | 签名失败日志覆盖              |
| 反射代码生成          | P3  | 从 DSL 生成 TReflected 注册 | 漏字段可能性→近零           | PoC→Gradual           |
| 远程延迟评估          | P3  | RTT Adaptive 算法        | 丢包率下吞吐保持            | 实测曲线                  |

---
## 19. 术语表 & 集成速览（增强长版）
| 术语               | 定义                      | 关键属性                 | 相关模块              |
|------------------|-------------------------|----------------------|-------------------|
| Element          | 场景数据逻辑单元                | Name/Type/Hash       | SceneElementsImpl |
| Actor            | 可层级组织 Element           | Transform / Children | Impl + XML        |
| Resource         | Mesh/Material/Texture 类 | 引用被 Actor 使用         | Impl              |
| Variant          | 属性差异集合                  | PropertyCaptures[]   | VariantImpl       |
| Capture          | 单属性快照                   | Category+Value       | VariantImpl       |
| PatchUpVersion   | 旧字段迁移函数                 | 版本判定+映射              | XmlReader         |
| DirectLink Batch | 增量消息集合                  | Seq / Msg[]          | SceneReceiver     |
| Mask (位掩码)       | 属性组变化标志                 | 位组合                  | DirectLink        |
| Layered Hash     | 分层 Hash 架构              | L0/L1/L2             | Hash 系统           |
| LegacyName       | 历史字段名                   | 列表                   | PatchUpVersion    |

### 19.1 集成流概览
```
DCC 翻译器 → (Factory + Impl + 反射字段) → Hash / XML / 二进制资源 → Unreal 导入层 (XmlReader + Patch) → DirectLink 持续增量 (Diff + Receiver) → 视图/渲染缓存
```

### 19.2 外部工具对接点
| 工具                   | 输入       | 输出        | 用例      |
|----------------------|----------|-----------|---------|
| audit_hash_fields.py | Impl 源码  | 缺失字段报告    | PR Gate |
| xml_roundtrip_diff   | 原/回导 XML | 差异统计      | 兼容检查    |
| perf_scan_scene      | 多样场景     | 耗时/内存 CSV | 性能基线    |
| dl_schema_dump       | 消息流      | 协议字段清单    | 协议文档更新  |

---
## 20. 核心 API 签名速览（增强长版）
> 汇总高频 API 调用形态；签名以头文件为准，此处为结构化速查与扩展注意点。

### 20.1 Factory / Scene 构建
| 场景       | 伪代码                                                             | 说明                 |
|----------|-----------------------------------------------------------------|--------------------|
| 创建基础资源   | `auto Mesh = FDatasmithSceneFactory::CreateMesh(NAME);`         | 仅占位，需 SetFile/LODs |
| 创建材质图    | `auto Mat = FDatasmithSceneFactory::CreateUEPbrMaterial(NAME);` | 表达式容器              |
| 创建 Actor | `auto A = FDatasmithSceneFactory::CreateMeshActor(NAME);`       | 需绑定 Mesh 名称        |
| 添加到场景    | `Scene->AddMesh(Mesh);` / `Scene->AddActor(A);`                 | 场景持有引用             |
| 复制场景     | `auto Clone = FDatasmithSceneFactory::DuplicateScene(Scene);`   | 深拷贝+需重算 Hash       |

### 20.2 Actor 常用接口
| API                      | 说明       | 注意             |
|--------------------------|----------|----------------|
| SetTransform(FTransform) | 位置/旋转/缩放 | 标记 HashDirty   |
| AddChild(Actor)          | 建立层级     | 防循环/重复添加       |
| SetVisibility(bool)      | 显隐       | DirectLink 掩码位 |
| AddTag / ResetTags       | 标签管理     | 排序再 Hash 保稳定   |

### 20.3 Mesh / Material
| API | 说明 | 注意 |
| Mesh->SetFile(Path) | 关联 .geom 或外部数据 | 应更新 FileHash |
| Mesh->SetLightmapCoordinateIndex(i) | 光照贴图通道 | 与导入器期望匹配 |
| UEPbrMaterial->AddMaterialExpression<T>() | 节点构造 | 图循环检测 |
| Expression->ConnectExpression(...) | 建立连线 | 指向输出索引稳定 |

### 20.4 Texture / Sampler
| API | 说明 | 注意 |
| Texture->SetFile(Path) | 指定纹理源 | 路径正规化 & Hash |
| Texture->SetColorSpace(Enum) | 颜色空间 | Hash 必含 |
| Sampler：Scale/Offset/Rotation | UV 操作 | 量化一致 |

### 20.5 Animation
| API | 说明 | 注意 |
| AddFrame(Type, FrameInfo) | 插入关键帧 | 排序+去重 |
| SetCurveInterpMode(Mode) | 插值模式 | 不支持模式回退 |
| GetFramesCount(Type) | 查询 | 空类型=0 |

### 20.6 Variant
| API | 说明 | 注意 |
| AddActorBinding(Variant, Binding) | 绑定 Actor | Actor 名稳定 |
| AddPropertyCapture(Binding, Capture) | 捕获属性 | 路径大小写规范 |
| ApplyVariant(Variant) | 重放 | 只写差异字段 |

### 20.7 DirectLink（伪）
```cpp
for(auto& Msg : Batch.Messages){
  switch(Msg.Type){
    case EDLType::Add:    ApplyAdd(Msg); break;
    case EDLType::Update: ApplyUpdate(Msg); break;
    case EDLType::Reparent: ApplyReparent(Msg); break;
    case EDLType::Remove: ApplyRemove(Msg); break;
  }
}
```

### 20.8 扩展提醒
| 修改              | 需同步点                                       |
|-----------------|--------------------------------------------|
| 新枚举(类型/子类型)     | Definitions / Factory / XML / Hash / Tests |
| 新表达式节点          | 枚举+Factory+材质图 Hash                        |
| 新 DirectLink 消息 | 协议枚举 / Receiver / 测试回放                     |

---
## 21. 典型调用序列（增强长版）
> 展示端到端操作链、失败回退与测试观察点。

### 21.1 导出流程
1) 解析外部场景 → 2) Factory 创建元素 → 3) 填充属性/材质图 → 4) Recalc Hash → 5) 写 XML/二进制资源 → 6) 统计日志。
失败回退：写文件失败→终止；部分资源缺失→占位并警告。
关键断言：Hash 稳定；输出文件数符合期望；默认值未冗余写入。

### 21.2 导入流程
1) XML Reader 构建骨架 → 2) Resolve 引用 → 3) PatchUpVersion → 4) 构建 UObjects → 5) 校验资源引用 → 6) 生成统计。
断言：旧文件成功迁移；引用无悬挂；Actor/资源数量匹配。

### 21.3 DirectLink 增量
收集消息(排序+去重) → 生成批 → 接收校验序号 → 应用（Add→Update→Reparent→Remove） → 标记刷新 → 统计。
约束：幂等；重复批重放 Hash 不变；乱序消息不破坏场景。

### 21.4 Variant 切换
选择 Variant → 遍历 ActorBinding → 重放属性捕获 (跳过不可用路径) → 累积 HashDirty → 一次刷新。
断言：切换前后仅差异属性变化；回切原 Variant 场景 Hash 还原。

### 21.5 动画播放
时间采样→查邻近帧→插值→应用 Transform/Visibility→材质或其它通道刷新（可选）→帧缓存。
断言：帧顺序单调；插值值在区间内；禁用通道不更新。

### 21.6 回滚策略示例
| 操作          | 失败条件  | 回滚      |
|-------------|-------|---------|
| Reparent    | 形成环   | 取消操作    |
| UpdateParam | 类型不匹配 | 忽略单字段   |
| AddElement  | 类型未知  | 丢弃 & 日志 |

### 21.7 RoundTrip 验证脚本要点
- 导出→导入→再导出：第二次导出与第一次（排序归一化后）差异仅限时间戳。
- Hash 前后相等；材质图拓扑一致；动画帧数一致。

---
## 22. 导入/差异化策略与优化建议（增强长版）

### 22.1 XML
| 问题     | 策略      | 指标        |
|--------|---------|-----------|
| 冗余默认值  | 省略默认    | 文件减幅 %    |
| 浮点噪声   | 精度裁剪/量化 | Diff 噪声降低 |
| 大文件解析慢 | 流式/分区并行 | 导入耗时      |

### 22.2 Hash / 增量
| 问题    | 策略           | 指标       |
|-------|--------------|----------|
| 频繁小改  | Debounce+合并  | 批平均大小    |
| 单字段抖动 | 量化阈值         | 忽略率      |
| 全量重算贵 | 分层 Hash / 脏组 | P95 重算时间 |

### 22.3 材质图
| 问题     | 策略            | 指标   |
|--------|---------------|------|
| 重复子图   | 子图 Hash 缓存    | 命中率  |
| 拓扑遍历发散 | Kahn 拓扑排序+排序键 | 遍历时间 |

### 22.4 Mesh 导入
| 问题            | 策略     | 指标   |
|---------------|--------|------|
| 大顶点数初始化慢      | 分块并行构建 | 加速比  |
| Lightmap 生成阻塞 | 延迟/异步  | 首帧时间 |

### 22.5 DirectLink
| 问题     | 策略     | 指标     |
|--------|--------|--------|
| 高频重复更新 | 值比较忽略  | 忽略率    |
| 批大小不稳  | 自适应阈值  | 批方差    |
| 大批应用卡顿 | 并行字段应用 | P95 应用 |

### 22.6 质量门 (Quality Gates)
| Gate      | 条件   | 行为     |
|-----------|------|--------|
| 枚举覆盖      | 100% | 阻断合并   |
| Hash 覆盖   | ≥98% | 警告/审批  |
| RoundTrip | 失败=0 | 阻断     |
| 性能回归      | >5%  | Review |

### 22.7 审计流程摘要
抽取字段 → Hash 使用集对比 → 差集报警 → 版本/LegacyNames 校验 → 产出 JSON 报告 → CI 阻断或警告。

---
## 23. DirectLink 消息格式与示例（增强长版）

### 23.1 结构
| 部分       | 内容                                        | 说明                |
|----------|-------------------------------------------|-------------------|
| Header   | Magic, Version, BatchSeq, MsgCount, Flags | Flags: 压缩/签名/量化策略 |
| Messages | 顺序消息                                      | Type 驱动解析         |
| Optional | HMAC / 校验摘要                               | 完整性               |

### 23.2 消息类型
| Type         | 描述      | 关键字段                              |
|--------------|---------|-----------------------------------|
| Add          | 新元素     | Type,Parent,CoreProps,InitialHash |
| Upd          | 参数更新    | Id,Mask,Fields[],NewHash          |
| Rep          | 重定位     | Id,NewParent,Order?               |
| Rem          | 删除      | Id                                |
| Bulk         | Hash 校准 | [Id,Hash]×N                       |
| BulkMatParam | 材质批量参数  | MaterialId,(NameHash,Value)×N     |

### 23.3 字段编码建议
| 字段       | 编码       | 注       |
|----------|----------|---------|
| Id       | VarInt   | 紧凑      |
| NameHash | 32-bit   | 避免重复字符串 |
| Float    | 16/32 量化 | 带策略位    |
| Color    | 3×16     | 线性空间    |

### 23.4 JSON 调试样例
```json
{"batch":412,"messages":[{"t":"Add","id":10,"etype":"MeshActor","parent":1,"core":{"mesh":"ChairLOD0"},"hash":"9a3f"},{"t":"Upd","id":10,"mask":5,"fields":[{"n":"Length","v":2.2}],"hash":"b04c"}]}
```

### 23.5 重放测试
| 测试            | 期望         |
|---------------|------------|
| Idempotent    | 重放 Hash 不变 |
| OutOfOrder    | 乱序被忽略/延迟   |
| MissingParent | 子排队等待父     |
| TruncatedBulk | 部分生效+告警    |

### 23.6 安全
| 项  | 策略          | 说明   |
|----|-------------|------|
| 篡改 | HMAC        | 密钥轮换 |
| 顺序 | 批序列单调       | 防回滚  |
| 版本 | FeatureBits | 向后兼容 |

### 23.7 指标与监控
| 指标             | 说明       |
|----------------|----------|
| BatchSize 分布   | 帮助调优节流策略 |
| DroppedMsgs    | 序号缺口统计   |
| ReplayHashDiff | 幂等性校验    |

### 23.8 工具
| 工具            | 用途              |
|---------------|-----------------|
| dl_inspect    | 批解析/人类可读        |
| dl_replay     | 重放日志 -> 最终 Hash |
| dl_proto_diff | 协议版本字段差异        |

