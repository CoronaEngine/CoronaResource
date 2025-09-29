# Datasmith Core 深度解析文档导航

> 更新说明（本次增强 2025-09-29）：
> - 新增：旧版本子节引用对照（兼容早期文档中 11.4 / 15.6 等编号）。
> - 新增：脚本输出格式建议 (JSON Schema 草案 & 示例)。
> - 新增：版本变更记录 / 搜索指引章节，便于快速定位。
> - 清理：去除可能引用不存在的子节号（保持“章节 + 关键词”定位方式）。
> - 统一：所有对主文档的引用均指向 `DataSmith解析.md`，旧文件 `DataSmith逐文件解析.txt` 已合并/弃用。
> - 补充：后续计划章节编号占位（25 并发模型 / 26 错误码与日志体系）说明。

> 速查：快速扩展示例与最小步骤仍参考 `QUICK_START_EXT.md`。

---
## 0. 搜索指引（全文检索建议）
| 需求 | 建议关键字 | 说明 |
|------|-----------|------|
| 轻量反射字段登记 | `TReflectedParam` 或 `Register(` | 查找参数注册位置 |
| Hash 实现 | `CalculateElementHash` | 确认字段是否参与 |
| 迁移兼容 | `PatchUpVersion` / `LegacyNames` | 旧字段映射 |
| DirectLink 位掩码 | `Mask` / `Bit` / `TransformBit` | 增量属性标识 |
| 批量材质参数 | `ParamBatch` | 材质参数聚合协议 |
| 分层 Hash | `L0` / `L1` / `RollingHash` | 分组与聚合 |
| Variant 捕获 | `PropertyCapture` | 变体差异点 |

---
## 1. 快速定位
| 目标 | 对应章节 | 说明 |
|------|----------|------|
| 整体架构 / 分层 | 1, 2 | 模块划分、命名约定 |
| 单文件职责概览 | 11 | Public / Private 逐文件表与要点 |
| 全量接口/符号索引 | 12, 20 | 符号 / API 签名速览 |
| 轻量反射机制 | 13 | 参数注册/量化/Hash/审计 |
| DirectLink 增量机制 | 14, 23 | 标脏→Diff→消息→应用 & 协议格式 |
| 扩展模板（示例） | 15 | 新 Actor / 表达式 / Capture / 压缩 |
| 回归与测试矩阵 | 16 | 功能/性能/兼容/Fuzz/覆盖率 |
| 变更影响与回滚 | 17 | 变更类型 → 风险 → 快速回退 |
| 演进路线 / KPI | 18 | 短/中/长期目标与指标 |
| 术语与概念索引 | 19 | 术语表/分层 Hash/RollingHash |
| 调用序列场景 | 21 | 导出/导入/增量/变体/动画/材质修改 |
| 导入/差异化策略 | 22 | 导入性能 / 差异化 / 内存 / 回退 |
| 自检与缺口列表 | 24 | 并发/内存/安全/脚本化缺口清单 |

---
## 1.1 旧版本子节引用对照（兼容手册）
| 旧引用示例 | 现在定位方式 | 说明 |
|-----------|-------------|------|
| 11.4 (Actor Impl) | 11 + 搜索 `Actor` 或 `SceneElementsImpl` | 细分被折叠进单表 |
| 11.6 (Hash 相关) | 13 + 搜索 `Hash 计算` | Hash 流程集中到 13 章 |
| 15.3 (Variant Capture) | 15 + 搜索 `Capture` | 模板多例合并 |
| 15.6 (材质表达式扩展) | 15 + 搜索 `表达式` / `ClearCoat` | 表达式注册示例 |
| 22.2 (增量策略) | 22 + 搜索 `增量` / `策略` | 策略表合并 |
| 22.6 (Hash 审计脚本) | 13 + 搜索 `审计脚本` | 审计集中于 13.9/13.10 |
| 23.x (协议结构) | 23 + 搜索 `Update 内部结构` | 协议章节精简重写 |

> 若历史提交/评审仍引用旧编号，按“章节 + 关键词”组合即可定位。

---
## 2. 建议阅读路径
| 目标 | 推荐顺序 | 说明 |
|------|----------|------|
| 首次全面了解 | 1 → 2 → 11 → 13 → 14 → 21 | 自上而下结构→机制→场景 |
| 立即做功能扩展 | 12 → 15 → 22 | 符号索引 + 模板 + 策略 |
| 增量/性能调优 | 13 → 14 → 22 → 16 | 反射&Hash→协议→性能策略→验证 |
| 发布回归 | 16 → 17 → 24 | 测试矩阵→风险映射→缺口确认 |

---
## 3. 常见任务跳转（章节 + 关键词）
| 任务 | 主文档章节 | 核心关键词 | 快速核对 |
|------|-----------|------------|----------|
| 新增 Actor 类型 | 11 / 15 / 13 | Factory / Impl / TReflected | 枚举+工厂+Hash+XML 对称 |
| 新材质表达式节点 | 11 / 15 / 13 | ExpressionType / Graph / Hash | 拓扑顺序可重现 |
| 新 Variant Property Capture | 11 / 15 / 13 | Capture / Category / Diff | RoundTrip + Hash 覆盖 |
| 新动画子类型 | 11 / 15 | SubType / Serializer | 帧排序+去重 |
| 增量协议扩展 | 14 / 23 | 位掩码 / 批处理 / Resync | Seq / 掩码准确 |
| 性能调优 | 13 / 14 / 16 / 22 | Quant / Batch / Baseline | P95 未回退 |
| 旧字段迁移 | 13 / 22 / 16 | LegacyNames / 迁移日志 | 旧资产全过 |
| Hash 覆盖审计 | 13 / 11 / 24 | AffectsHash / Missing | 无漏字段提示 |
| RoundTrip 检查 | 11 / 16 | 默认值省略 / PatchUpVersion | Diff = 空 |

---
## 4. 自动化脚本（建议放置 `tools/` 下）
| 脚本 | 目的 | 核心逻辑要点 | 输出异常示例 |
|------|------|--------------|--------------|
| audit_factory_enum.py | 枚举覆盖 | 解析枚举 + 工厂/注册表 | Missing enum: MeshAreaLight |
| audit_hash_fields.py | Hash 覆盖 | Impl 字段 vs Hash 函数 | Missing hash fields: Radius,Length |
| roundtrip_verify.py | XML 对称 | 导出→导入→再导出 Diff | Diff: Extra attr IntensityLm |
| audit_legacy_usage.py | 迁移清理 | LegacyNames 使用性 | Unused legacy name: OldExposure |
| perf_snapshot.py(可选) | 性能采集 | Hash/Export/Import 计时 | Hash P95 regression +23% |

### 4.1 标准脚本输出（建议 JSON）
```json
{
  "tool": "audit_hash_fields",
  "version": 1,
  "timestamp": "2025-09-29T10:15:00Z",
  "status": "fail",
  "issues": [
    {"class": "FDatasmithTubeLightActorElementImpl", "missingFields": ["Radius"]}
  ]
}
```

### 4.2 最小 JSON Schema（伪）
```json
{
  "type":"object",
  "required":["tool","status","issues"],
  "properties":{
    "tool":{"type":"string"},
    "status":{"enum":["ok","fail"]},
    "issues":{"type":"array","items":{"type":"object"}}
  }
}
```

---
## 5. 最低开发工作流 Checklist
1. 新增/修改代码（枚举、接口、Impl、XML）。  
2. 运行脚本：工厂覆盖 + Hash 字段 + RoundTrip。  
3. 执行回归：按变更类型映射 17 章（最小集合）。  
4. 性能抽样：Hash / Import / Export 与基线比较。  
5. 文档：更新 `DataSmith解析.md` 的相关章节条目。  
6. 变更风险：写入 17 章映射表或提交描述。  
7. Review 核对：24 章缺口（是否影响并发/安全/内存）。  
8. 最终脚本输出 JSON 均 status=ok。  

---
## 6. FAQ 精简
| 问题 | 简答 | 详见 |
|------|------|------|
| DirectLink 不刷新 | 未登记哈希或未 MarkHashDirty | 13 / 14 / 22 |
| 材质图 Hash 抖动 | 遍历顺序/浮点未量化 | 13 / 22 |
| XML 文件变大 | 默认值未省略 (Writer) | 11 / 22 |
| 动画帧错乱 | 插入未排序去重 | 11 / 16 |
| 旧文件字段缺失 | 漏 PatchUpVersion | 13 / 16 |
| 增量带宽高 | 缺批处理/量化 | 14 / 22 |

---
## 7. 后续推荐行动（对应 24 章缺口）
| 主题 | 说明 | 建议落地 | 优先级 |
|------|------|----------|--------|
| 并发模型文档 | 缺线程/锁策略矩阵 | 25 章：共享结构→访问→锁 | 高 |
| 错误码 & 日志等级 | 缺统一分层 | 26 章：错误分类/恢复策略 | 高 |
| 性能脚本标准化 | 基线采集不统一 | JSON + 周期趋势 | 中 |
| 内存布局分析 | 缺结构体数据 | 采集节点/元素均值 | 中 |
| 安全协议 | 协议未鉴权 | FeatureBits + HMAC | 中 |
| 动画压缩策略 | 仅规划 | 曲线拟合/量化误差实验 | 低 |
| Schema 生成 PoC | 仅设想 | 枚举/字段→代码生成 | 低 |

---
## 8. 与 Quick Start 的区别
| 文档 | 侧重 | 适用读者 |
|------|------|----------|
| 本导航 (DOC_README) | 章节索引 + 流程索引 | 快速跳转 / 维护者 |
| QUICK_START_EXT.md | 最小实现步骤 | 首次扩展 / 快速接入 |
| DataSmith解析.md | 体系化深度内容 | 架构/高级优化 |

---
## 9. 术语微索引
| 术语 | 快速释义 | 章节 |
|------|----------|------|
| L0/L1/L2 Hash | 分层属性与元素聚合 Hash | 13 / 14 |
| ParamBatch | 聚合材质参数 Update | 15 / 23 |
| LegacyNames | 旧 XML 字段别名集合 | 13 |
| QuantStep | 浮点量化步长 | 13 / 22 |
| ResyncRequest | 全量校准消息 | 23 |
| RollingHash | 场景聚合校验值 | 13 / 14 |

---
## 10. 快速提交前统一 Checklist
- [ ] 枚举 + 工厂 / 注册表覆盖（无 Missing）。
- [ ] 影响渲染/同步字段：TReflected + AffectsHash。 
- [ ] Setter 全部 MarkHashDirty。 
- [ ] XML Writer/Reader 对称（默认值省略策略已验证）。
- [ ] RoundTrip Diff 仅允许时间戳/排序差异。 
- [ ] DirectLink 位掩码更新正确，微抖动被量化过滤。 
- [ ] 变更影响映射（17）已更新，回滚方案可执行。 
- [ ] 性能指标未超阈值（16 基线）。
- [ ] 24 章缺口风险评估已记录。 
- [ ] 审计脚本 JSON 输出全 green。

---
## 11. 版本变更记录 (Changelog)
| 日期 | 版本标签 | 要点 | 说明 |
|------|----------|------|------|
| 2025-09-29 | vEnhanced-2 | 加入旧引用对照/脚本输出/Changelog | 同步主文档 13~23 增强 |
| 2025-09-28 | vEnhanced-1 | 重构导读结构，去除过时文件引用 | 替换旧 DataSmith逐文件解析.txt |
| 2025-09-27 | vLegacy | 旧多子节硬编码引用版本 | 已弃用 |

> 未来新增：25 (并发模型) / 26 (错误码日志) 若落地将追加在此。

---
如需：
- 添加“并发模型/错误码”新章 → 主文档顺延 25 / 26 章。
- 引入统一审计脚本 schema → 将在 18 章路线补 KPI。 

> 反馈：在 Issue / MR 中引用“章节 + 关键词”（如 `#13 量化策略`）即可精确定位。
