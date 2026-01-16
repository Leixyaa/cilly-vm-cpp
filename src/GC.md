
### A. GC 核心算法与对象管理

* **Mark–Sweep（标记-清扫）**：通过可达性遍历标记存活对象，再清扫不可达对象并释放内存。
* **intrusive all_objects 链表**：所有 GC 管理的对象都挂在 `all_objects_` 单链表上（`GcObject::next`），Sweep 时从链表摘除。
* **颜色/灰栈遍历（DrainGrayStack）**：支持递归对象图的非递归遍历（灰栈/工作栈），避免深递归爆栈。
* **对象自描述 Trace**：每个运行时对象实现 `Trace(Collector&)`，由对象自己决定要标记哪些引用字段，形成“对象图遍历协议”。

---

### B. Roots 体系

#### B1. B 方案：外部 roots 回调

* `CollectWithRoots(trace_roots_callback)`：GC 不知道 VM 内部结构，由调用方提供“如何标记 roots”的回调。
* 用于单测和 bring-up：可以在 gtest 中构造对象图并验证 Sweep 结果，**不依赖 VM 完整实现**。

#### B2. A 方案：VM Roots 扫描

收口完成后，VM 侧提供 `VM::TraceRoots(Collector&)`，GC 通过回调调用 VM roots 扫描。roots 预计覆盖：

* **运行栈 values**（脚本执行的 Value 栈）
* **CallFrame / locals**（活跃调用帧局部变量槽）
* **活跃帧常量池 const pool**（active frames 的 Chunk 常量）
* **全局/模块级变量容器**（globals / module globals，如果有）
* **VM 常驻对象**

  * intern string / string pool（如果存在）
  * callable 表（user functions / native / class/bound method 的持有结构）
  * method table / class methods（如果这些结构里持有 Obj/Value）
* **临时 root（危险窗口期）**

  * RootGuard 或 VM 内部 root 栈，用于“构造对象但尚未存入稳定容器/栈槽”的窗口保护

> 你前面做的“native argv pinning / 在 native 内触发 GC”属于 roots 收口的一部分：确保在进入 native 时参数 Value 里的 Obj 不会被 sweep。

---

### C. 运行时对象图 Trace 覆盖

* **List（ObjList）**：遍历 `vector<Value>`，Value 是 Obj 时标记
* **Dict（ObjDict）**：遍历 `unordered_map<string, Value>` entries，Value 是 Obj 时标记
* **Class（ObjClass）**：标记 `superclass`，并（视实现）标记 method table 关联的 callable/函数对象
* **Instance（ObjInstance）**：标记 `klass`，并标记 fields（通常是 ObjDict/Value 容器）
* **BoundMethod（ObjBoundMethod）**：标记 receiver（Value）并保持 method 可调用
* **（如果你有 ObjString/ObjFunction 等）**：各自 Trace 它们引用的对象（比如字符串池/常量等）

---

### D. 自动触发策略

1. **对象数量阈值触发（object-count threshold）**

* `object_count_` 超过阈值就触发 Collect（你现在调过阈值，测试也验证过“阈值过大不会触发”的用例）

2. **堆字节预算触发（heap-bytes budget / bytes-budget）**

* 维护 `heap_bytes_`（估算堆占用）
* 支持在容器增长（list/dict 扩容）时通过 `AddHeapBytesDelta(delta)` 增量更新预算
* 支持“大对象少数量也触发 GC”的场景（你写了 large object smoke test）

3. **触发点收敛到 VM 的少数安全点（收口步骤会做）**

* 不在每条指令都计算/检查，而是：

  * 分配/扩容时设置 `gc_poll_ = true`
  * 下一条安全点统一 `MaybeCollectGarbage_()`
* 达到：**正确性不退化，性能开销可控**

---

### E. 可测试性与调试可观测性

* **__gc_collect() builtin**：脚本中显式触发 GC（用于构造“最危险窗口期”的回归测试）
* **native 内触发 GC 的保活测试**：验证 argv pinning / roots 正确
* **统计指标（用于断言）**：

  * `last_swept_count()`：最近一次 sweep 回收数量
  * `total_swept_count()`（如果你加了）：累计回收数量
  * `heap_bytes()`（如果暴露了）：预算变化
* **gtest 全链路回归**：脚本运行 + 触发 GC + 验证不崩溃、结果正确、确实发生回收（swept > 0）

---

### F. 安全性/工程策略

* **先 B 方案 bring-up，再逐步 A 方案接管**：避免“GC 上来就扫 VM 全部结构导致漏根/误根”
* **RootGuard/临时 roots 边界**：明确“哪些地方必须临时保活”，减少 UAF 风险
* **侵入式链表 Sweep 摘除**：回收路径清晰、易于统计和测试


