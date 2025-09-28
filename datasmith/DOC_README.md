# Datasmith Core 深度解析文档导航

> 附：快速扩展速查请参见 `QUICK_START_EXT.md`（新增元素/表达式/DirectLink 最小实现步骤）。

本仓库内 `DataSmith逐文件解析.txt` 为完整（>22 节）深度解析。此文件提供速览与跳转索引。

## 1. 快速定位
| 目标       | 对应章节          | 说明                         |
|----------|---------------|----------------------------|
| 整体架构/分层  | 1, 2          | 模块划分、命名约定                  |
| 单文件职责    | 11            | Public / Private 逐文件表      |
| API 全量速查 | 12, 20        | 接口/枚举/常用调用签名               |
| 反射与增量机制  | 13, 14        | 参数反射层 / DirectLink Hash 协同 |
| 典型调用流程   | 21            | 导出/导入/DirectLink/动画        |
| 优化与策略    | 22            | 导入性能与差异化策略                 |
| 扩展示例模板   | 15            | 新类型 / 表达式 / 场景字段 / 测试模板    |
| 回归测试矩阵   | 16            | 功能域 × 用例 ID × 断言           |
| 变更影响映射   | 17            | 修改 → 受影响域 → 回归列表           |
| 演进路线     | 18            | 中长期改进建议优先级                 |
| 术语与集成    | 19            | 术语表 & 集成速览                 |
| 快速扩展速查   | QuickStart 文档 | 参见 `QUICK_START_EXT.md`    |

## 2. 建议阅读路径
1. (首次) 1→2→11→13→14 了解核心机制。
2. (开发扩展) 12→15→22 直接按模板落地。
3. (性能或增量调优) 14→22→18。
4. (回归发布) 16→17 Checklist 化执行。

## 3. 常见任务跳转
| 任务              | 快速步骤                               | 参考                     |
|-----------------|------------------------------------|------------------------|
| 新增 Actor 子类     | 定义枚举+接口 → 工厂分支 → Impl + Hash + XML | 11.4, 15.3, 22.3       |
| 材质表达式扩展         | 枚举 → 工厂 → Impl(Hash) → XML(可选)     | 15.6, 13.9             |
| DirectLink 性能降噪 | 属性节流 + 批量消息                        | 14.4, 22.2             |
| Hash 覆盖审计       | 列出 Impl 字段 vs Hash 代码              | 13.3, 11.6, 22.6       |
| 旧版本文件兼容         | PatchUpVersion 流程 + 测试             | 13.6, 16(XML-PATCH-01) |

## 4. 可选自动化脚本（占位思路）
建议在 `tools/` 目录添加脚本：
- `audit_factory_enum.py`：解析 `DatasmithDefinitions.h` 与 `DatasmithSceneFactory.cpp`，找出未覆盖枚举。
- `audit_hash_fields.py`：扫描 `*Impl.h` 中 TReflected<> 字段列表，对比对应类的 `CalculateElementHash` 实现（正则粗匹配 MD5.Update 关键字）。
- `roundtrip_verify.py`：导入 → 重新导出 → 对比筛选字段（忽略版本与时间戳）。

伪逻辑示例（hash 审计伪代码）：
```python
import re, pathlib
root = pathlib.Path('Datasmith_runtime/DatasmithCore/Private')
impl_text = (root/'DatasmithSceneElementsImpl.h').read_text(encoding='utf-8', errors='ignore')
classes = re.findall(r'class\s+(FDatasmith\w+Impl)[^{]*{([^}]*)};', impl_text, re.S)
for name, body in classes:
    fields = re.findall(r'TReflected<[^>]+>\s+(\w+);', body)
    hash_fn = re.search(r'%s::CalculateElementHash[^{]*{([^}]*)}' % name, impl_text, re.S)
    missing = []
    if hash_fn:
        code = hash_fn.group(1)
        for f in fields:
            if f not in code and 'Hash' not in f and not f.startswith('bUse'):
                missing.append(f)
    if missing:
        print(name, 'possibly missing hash fields:', missing)
```
（上例只作启发，真实工程需多文件拼接并兼顾子类委托 Hash。）

## 5. 最低工作流程（开发→测试→提交）
1. 变更代码 / 新类型。
2. 运行审计脚本（枚举覆盖 + Hash 字段 + RoundTrip）。
3. 执行回归子集（按 17 映射）。
4. 更新 `DataSmith逐文件解析.txt`（若涉及公共接口或流程）。
5. Code Review：确认 Checklist (22.7) 全部打勾。

## 6. FAQ 精简
| 问题                       | 简答                       | 详见                    |
|--------------------------|--------------------------|-----------------------|
| 为什么某字段改了 DirectLink 不刷新？ | 未注册 TReflected 或未纳入 Hash | 13 / 14 / 22.2        |
| 材质图 Hash 不稳定？            | 遍历顺序不固定或浮点未规范化           | 13.9 / 22.3           |
| XML 文件变大？                | 默认值未省略或调试模式写注释           | 11.7 / 22.1           |
| 动画帧错乱？                   | 添加帧未排序去重                 | 11.6 / 16(ANIM-TR-01) |

## 7. 后续推荐行动（若尚未实施）
- 引入脚本化 Hash 覆盖 CI Gate（参阅 22.5, 22.6）。
- 工厂注册机制（插件式）减少冲突（18 高优推荐）。
- 分层 Hash 与属性位掩码支持（14.6 / 22.3）。
- 材质子图 Hash 缓存提升大图迭代体验（13.9 / 18）。
- 生成 JSON Schema 支撑外部校验（13.11 / 18）。

---
如需补充专门章节（例如：材质表达式 DSL 规范、DirectLink 协议格式），可在主文档 23+ 节继续追加。
