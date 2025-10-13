# Datasmith 扩展快速上手精简指南

> 面向需要在最短时间内完成功能扩展（新增元素 / 材质表达式 / DirectLink 增量支持 / 新场景字段）的开发者。主文档详解参见 `DataSmith解析.md`（章节 11~23）。本指南强调最小步骤 + Checklist。

---
## 1. 适用场景
| 需求               | 示例                      | 推荐阅读路径             |
|------------------|-------------------------|--------------------|
| 新增 Actor / 资源类型  | SplineActor / TubeLight | 11.4, 13, 15, 23   |
| 新增材质表达式节点        | TimeSine / Noise        | 12.3.9, 13.9, 15.6 |
| 新增场景级配置字段        | ExposureBias            | 15.2, 13.7         |
| DirectLink 自定义同步 | 属性位掩码 / 分层 Hash         | 14, 23             |
| Hash 覆盖审计        | 防遗漏                     | 11.6, 13.3, 22.6   |

---
## 2. 环境前提（最小）
1. 能编译 DatasmithCore（确认 Build.cs 已包含你的私有新文件）。
2. 熟悉 `DatasmithDefinitions.h` 与 `DatasmithSceneFactory.cpp`。
3. 明白 `IDatasmithSceneElements.h` 中接口模式（资源 vs Actor）。
4. 配好测试场景（小型：一个 Mesh + 一个材质 + 一个灯光）。

---
## 3. 通用扩展工作流（统一 Checklist）
1. 定义：枚举 / 接口 / Impl 类骨架。
2. 工厂：在 `DatasmithSceneFactory.cpp` 添加 case。
3. 反射：`TReflected<>` 字段注册（保证 Hash 脏标记）。
4. XML：Writer 写非默认；Reader 读 + Patch 旧名。
5. Hash：序列化所有“会影响外观/逻辑”字段（顺序稳定）。
6. DirectLink（可选）：支持 Add/Update/Remove/Reparent。
7. 测试：RoundTrip / Hash 变更 / DirectLink Diff / 脚本审计。
8. 文档：在 `DataSmith解析.md` 与变更记录追加说明。

---
## 4. 新增 Element 类型（5 分钟模板）
最少修改文件：
- `DatasmithDefinitions.h`: `EDatasmithElementType::MyCustom = 1ull << N`
- `IDatasmithSceneElements.h`: 接口（继承 Actor 或资源基类）
- `DatasmithSceneFactory.cpp`: switch 分支返回 Impl
- `DatasmithSceneElementsImpl.(h/cpp)` 或 新建 `DatasmithMyCustomImpl.(h/cpp)`
- XML 读写：添加节点 `<MyCustom .../>`
- Hash：在 Impl::CalculateElementHash 中序列化字段

示例字段：
```
TReflected<FVector> SplinePoint;    // 若多点，用 TArray<FVector>
TReflected<float>  Width;
```
注意：数组大时考虑量化或局部 Hash。

---
## 5. 新增材质表达式节点（示例：TimeSine）
步聚速览：
1. 枚举：`EDatasmithMaterialExpressionType::TimeSine`。
2. 接口（可选，若需要特定参数访问器）。
3. Impl：`FDatasmithMaterialExpressionTimeSineImpl` 注册 `Frequency`。
4. 工厂：`CreateMaterialExpression` 分支。
5. Hash：加入 `Frequency` 字节。
6. XML：`<Expr Type="TimeSine" Frequency="1.0"/>`。
7. 测试：不同 Frequency → Hash 不同；连接 Emissive 输入可见效果差异。

---
## 6. 新增场景级配置字段（ExposureBias）
1. Impl：在 Scene Impl 添加 `TReflected<float> ExposureBias;` 默认 0。
2. 接口：`Get/SetExposureBias`。
3. XML：Scene 根节点属性 `ExposureBias="0"`（省略默认 0）。
4. 导入后在 Importer 中应用到 PostProcessVolume 或全局设置。
5. Hash：若影响渲染出图则纳入；否则可延迟纳入下一版本。

---
## 7. DirectLink 增量最小实现
| 步          | 要点                               | 参考         |
|------------|----------------------------------|------------|
| Diff 收集    | 上一帧 HashMap vs 当前                | 14.2       |
| 属性位掩码      | transform/material/visibility... | 23.2, 23.7 |
| 消息顺序       | 资源先于 Actor                       | 23.5, 23.6 |
| Pending 引用 | Actor->Mesh 缺失排队                 | 23.11      |
| Ack / 重放   | seq 去重                           | 23.10      |
| 批量         | 聚合小改动                            | 23.9       |
| 阈值过滤       | 摄像机抖动 epsilon                    | 23.11      |

最小 JSON Update：
```json
{ "seq":10, "type":"Update", "payload":{
  "elementId":"Actor_Cam",
  "hash":"A1..",
  "attributeMask":1,
  "changed":{"transform":{"t":[0,0,120]}}
}}
```

---
## 8. Hash 覆盖与测试脚本
快速脚本顺序：
```
python tools/audit_factory_enum.py
python tools/audit_hash_fields.py
python tools/roundtrip_verify.py --scene sample_scene.udatasmith
```
断言：输出不含 Missing / Hash mismatch；RoundTrip 差异仅限时间戳或排序允许字段。

---
## 9. 常见错误速查
| 现象                 | 可能原因                  | 快速排查                             |
|--------------------|-----------------------|----------------------------------|
| 新类型导出后导入缺失         | XML 未写节点 / 枚举未注册工厂    | grep 类型名 & factory switch        |
| 改属性 DirectLink 不刷新 | 字段未纳入 Hash / 未标 Dirty | 检查 Set 调用 & CalculateElementHash |
| 材质图 Hash 不稳定       | 节点遍历顺序不固定             | 确认拓扑排序或固定 DFS 顺序                 |
| 场景重建过慢             | 小改动触发全重建              | 引入分层 Hash / 属性掩码                 |
| 旧版本文件读错值           | 少 PatchUpVersion      | 补映射并测试 XML-PATCH 用例              |

---
## 10. 30 分钟综合示例（步骤大纲）
1. 新枚举：`TubeLight`。
2. 新接口：强度角度参数。
3. Impl：字段 + Hash + 默认值。
4. 工厂分支。
5. XML 读写。
6. 简易 Translator 生成一个 TubeLight → 导出。
7. 导入验证属性正确。
8. DirectLink：改角度 + 发送 Update，日志验证 seq & hash 改变。
9. Audit 脚本通过。
10. 文档：QuickStart + 主文档 15.x 追加条目。

---
## 11. 主文档章节映射
| 快速任务          | 主文档章节              |
|---------------|--------------------|
| 新 Element     | 11.4, 13.3, 15.3   |
| 新表达式          | 12.3.9, 13.9, 15.6 |
| 新场景字段         | 13.7, 15.2         |
| DirectLink 协议 | 14, 23             |
| Hash 最佳实践     | 11.6, 13.3, 22.3   |
| 测试矩阵          | 16, 23.14          |

---
## 12. 快速提交前最终 Checklist
- [ ] 工厂 switch 有分支 & 枚举不冲突。
- [ ] Hash 计算包含新字段（或已记录不纳入理由）。
- [ ] XML Writer/Reader 对称 & 默认值省略。
- [ ] RoundTrip 一致；Diff 覆盖。
- [ ] DirectLink（若适用）Add/Update/Remove 路径走通。
- [ ] 文档（主文档 + QuickStart）已更新。
- [ ] 回归矩阵相关用例已执行（或列出待补）。

> 结束：如需更详尽协议与数据结构说明，跳转 `DataSmith解析.md` 第 11~23 节；如需增量消息字段示例查阅第 23 节。

