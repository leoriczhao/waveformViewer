# Waveform Viewer 渲染管线优化计划

## 概述

本计划针对 waveformViewer 渲染管线的三大性能瓶颈进行优化：

1. **DrawPass 优化**：通过排序和批处理减少状态切换
2. **多线程录制**：并行化图层更新
3. **Pipeline State 缓存**：缓存 VAO/VBO，跟踪 GL 状态，批量文本渲染

---

## 当前架构分析

### 渲染管线流程

```
Canvas → Device → Recorder → Recording (DrawOp[]) → Context (GlContext)
```

### 三层系统

| 层 | 内容 | 更新频率 |
|---|------|---------|
| `staticLayer_` | 背景、时间刻度、信号名 | 平移/缩放/选择时 |
| `waveformLayer_` | 波形绘制 | 平移/缩放/进制变更时 |
| `overlayLayer_` | 光标、当前值 | 光标移动时 |

### 当前性能瓶颈

| 瓶颈 | 位置 | 影响 |
|------|------|------|
| **文本渲染** | `gl_context.cpp:314-336` | 每字符一次 `glDrawArrays` |
| **频繁刷新** | `gl_context.cpp:297-298, 376-377` | 每次裁剪/文本变更触发刷新 |
| **无状态缓存** | `GlContext` 全局 | 冗余 OpenGL API 调用 |
| **DrawOp 内存** | `recording.hpp:11-30` | 72+ 字节/操作，堆分配 |
| **单线程** | `waveform_viewer.cpp` | 所有录制在主线程 |

---

## Phase 1: 文本批处理（最高优先级）

**目标**：将每字符绘制合并为单次绘制调用

### 当前问题

```cpp
// gl_context.cpp:314-336
for (char ch : op.text) {
    // ... 每字符生成四边形
    glBufferData(GL_ARRAY_BUFFER, sizeof(verts), verts, GL_STREAM_DRAW);
    glDrawArrays(GL_TRIANGLES, 0, 6);  // 每字符一次调用！
}
```

### 实现任务

| ID | 任务 | 描述 | 验证方式 |
|----|------|------|---------|
| 1.1 | 创建 `TextVertex` 结构 | `{f32 x, y, u, v}` | 编译通过 |
| 1.2 | 添加 `textVertices_` 缓冲 | `std::vector<TextVertex>` | 编译通过 |
| 1.3 | 实现 `flushText()` | 单次 `glDrawArrays` 绘制所有文本 | 单元测试 |
| 1.4 | 重构 `applyOp(Text)` | 累积四边形而非立即绘制 | 视觉测试 |
| 1.5 | 处理颜色变化 | 颜色变化时刷新，或使用顶点颜色 | 视觉测试 |
| 1.6 | 预分配 VBO | 使用估算大小的 `glBufferData` | 性能测试 |

### 成功标准

- 文本渲染使用 **每颜色 1 次绘制调用**（而非每字符 1 次）
- 预期改进：文本密集帧 **10-50 倍** 绘制调用减少

### 风险评估

| 风险 | 可能性 | 影响 | 缓解措施 |
|------|--------|------|---------|
| 需要每字符颜色 | 低 | 中 | 添加顶点颜色属性 |
| 批处理中途 Atlas 更新 | 低 | 高 | Atlas 更新前刷新 |

### 实现建议

- **Category**: `quick`
- **Skills**: 无需特殊技能
- **预估工时**: 2-3 小时

---

## Phase 2: DrawPass 排序与批处理

**目标**：通过排序 DrawOp 最小化状态切换

### 当前问题

```
FillRect → SetClip → Text → ClearClip → FillRect → SetClip → Text → ...
```

### 目标状态

```
[Clip A 内所有 FillRect] → [Clip A 内所有 Text] → [Clip B 内所有 FillRect] → ...
```

### SortKey 设计（借鉴 Skia）

```cpp
struct SortKey {
    u64 key;
    
    // 编码：高位 = 高优先级分组
    // [63:48] clip_id      - 按裁剪区域分组
    // [47:40] op_type      - 分离 FillRect, Line, Text
    // [39:8]  color_hash   - 相同颜色分组
    // [7:0]   sequence     - 组内保持原始顺序
    
    static SortKey make(u16 clipId, DrawOp::Type type, Color c, u8 seq);
    bool operator<(const SortKey& o) const { return key < o.key; }
};
```

### 实现任务

| ID | 任务 | 描述 | 验证方式 |
|----|------|------|---------|
| 2.1 | 设计 `SortKey` | 64 位键编码 | 设计文档 |
| 2.2 | 添加 `DrawPass` 类 | `Recording` → 排序后的 `DrawOp` 序列 | 单元测试 |
| 2.3 | 实现裁剪跟踪 | 录制时分配裁剪 ID | 单元测试 |
| 2.4 | 实现稳定排序 | 按 `SortKey` 排序，同键保持顺序 | 单元测试 |
| 2.5 | 集成到 `submit()` | 从 `Recording` 创建 `DrawPass` 并执行 | 视觉测试 |
| 2.6 | 添加指标 | 统计排序前后状态切换次数 | 性能测试 |

### 成功标准

- 状态切换减少 **50%+**
- 无视觉回归（稳定排序保持组内绘制顺序）

### 风险评估

| 风险 | 可能性 | 影响 | 缓解措施 |
|------|--------|------|---------|
| 排序开销超过收益 | 中 | 中 | 性能分析；小录制跳过排序 |
| 绘制顺序错误 | 中 | 高 | 全面视觉测试 |

### 实现建议

- **Category**: `unspecified-high`
- **Skills**: 无需特殊技能
- **预估工时**: 4-6 小时
- **依赖**: Phase 1（文本批处理使排序更有效）

---

## Phase 3: OpenGL 状态缓存

**目标**：通过跟踪当前状态消除冗余 OpenGL API 调用

### 当前问题

```cpp
void flushTriangles() {
    glUseProgram(shader_);           // 每次刷新都调用
    glUniform2f(resolutionLoc_, ...); // 每次刷新都调用
    glBindVertexArray(vao_);          // 每次刷新都调用
}
```

### GlStateCache 接口设计

```cpp
class GlStateCache {
public:
    void invalidate();  // beginFrame() 时调用
    
    void useProgram(u32 program);
    void bindVao(u32 vao);
    void bindVbo(u32 vbo);
    void setScissor(bool enabled, i32 x, i32 y, i32 w, i32 h);
    void setUniform2f(i32 loc, f32 x, f32 y);
    void setUniform4f(i32 loc, f32 x, f32 y, f32 z, f32 w);
    void bindTexture(u32 unit, u32 texture);
    
private:
    u32 currentProgram_ = 0;
    u32 currentVao_ = 0;
    u32 currentVbo_ = 0;
    bool scissorEnabled_ = false;
    Rect scissorRect_ = {};
    std::unordered_map<i32, std::array<f32, 4>> uniforms_;
    std::array<u32, 8> boundTextures_ = {};
};
```

### 实现任务

| ID | 任务 | 描述 | 验证方式 |
|----|------|------|---------|
| 3.1 | 创建 `GlStateCache` 类 | 跟踪绑定的程序、VAO、VBO、uniform、scissor | 单元测试 |
| 3.2 | 实现缓存 `useProgram()` | 仅在程序变化时调用 `glUseProgram` | 性能测试 |
| 3.3 | 实现缓存 `bindVao()` | 仅在 VAO 变化时调用 `glBindVertexArray` | 性能测试 |
| 3.4 | 实现缓存 `setScissor()` | 仅在矩形变化时调用 `glScissor` | 性能测试 |
| 3.5 | 实现缓存 uniform | 跟踪 uniform 值，跳过冗余上传 | 性能测试 |
| 3.6 | 添加 `invalidate()` 方法 | 重置缓存状态（帧开始时调用） | 单元测试 |
| 3.7 | 集成到 `GlContext` | 用缓存版本替换直接 GL 调用 | 视觉测试 |

### 成功标准

- 每帧 GL API 调用减少 **50%+**
- 可通过 `apitrace` 或手动插桩测量

### 风险评估

| 风险 | 可能性 | 影响 | 缓解措施 |
|------|--------|------|---------|
| 缓存失效错误 | 中 | 高 | 全面测试；保守失效策略 |
| 外部 GL 状态变化 | 低 | 中 | 文档化假设；上下文切换时失效 |

### 实现建议

- **Category**: `quick`
- **Skills**: 无需特殊技能
- **预估工时**: 3-4 小时
- **依赖**: 独立（可与 Phase 2 并行）

---

## Phase 4: DrawOp 内存优化

**目标**：减少内存分配，改善缓存局部性

### 当前问题

```cpp
struct DrawOp {
    Type type;
    Rect rect;
    Point p1, p2;
    std::vector<Point> points;  // 堆分配
    std::string text;           // 堆分配
    Color color;
    f32 width;
};
// 72+ 字节，可能有堆分配
```

### CompactDrawOp 设计

```cpp
struct CompactDrawOp {
    DrawOp::Type type;      // 1 字节
    u8 padding[3];
    Color color;            // 4 字节
    f32 width;              // 4 字节
    
    union {
        struct { Rect rect; } fill;                    // 16 字节
        struct { Point p1, p2; } line;                 // 16 字节
        struct { u32 offset; u16 count; } polyline;   // 6 字节
        struct { Point pos; u32 offset; u16 len; } text; // 14 字节
        struct { Rect rect; } clip;                    // 16 字节
    } data;                 // 16 字节
};
// 总计: 28 字节（vs 72+ 字节）
```

### 实现任务

| ID | 任务 | 描述 | 验证方式 |
|----|------|------|---------|
| 4.1 | 设计 `DrawOpArena` | DrawOp 数据的线性分配器 | 单元测试 |
| 4.2 | 用 `StringRef` 替换 `std::string` | 存储 arena 中的偏移+长度 | 单元测试 |
| 4.3 | 用 `PointsRef` 替换 `std::vector<Point>` | 存储偏移+计数 | 单元测试 |
| 4.4 | 实现 `CompactDrawOp` | 固定大小结构（~32 字节）带引用 | 单元测试 |
| 4.5 | 更新 `Recorder` | 使用 arena 分配 | 单元测试 |
| 4.6 | 更新 `Recording` | 拥有 arena，提供访问器 | 单元测试 |
| 4.7 | 更新 `GlContext::applyOp` | 使用新访问器 | 视觉测试 |

### 成功标准

- DrawOp 大小从 **72+ 字节减少到 ~32 字节**
- 录制期间零堆分配（arena 预分配）
- 改善缓存局部性（通过 `perf stat` 测量）

### 实现建议

- **Category**: `unspecified-high`
- **Skills**: 无需特殊技能
- **预估工时**: 6-8 小时
- **依赖**: 应在 Phase 1-3 之后（优先级较低）

---

## Phase 5: 多线程录制（未来）

**目标**：跨工作线程并行化图层更新

### 设计概念

```
┌─────────────────────────────────────────────────────────────┐
│                      主线程                                  │
│  ┌─────────┐    ┌─────────┐    ┌─────────┐                  │
│  │ Static  │    │Waveform │    │ Overlay │  ← 录制          │
│  │ Layer   │    │ Layer   │    │ Layer   │    (CPU 工作)    │
│  └────┬────┘    └────┬────┘    └────┬────┘                  │
│       │              │              │                        │
│       └──────────────┼──────────────┘                        │
│                      ▼                                       │
│              ┌──────────────┐                                │
│              │   GlContext  │  ← GL 执行（必须单线程）       │
│              │   submit()   │                                │
│              └──────────────┘                                │
└─────────────────────────────────────────────────────────────┘
```

### OpenGL 限制

- OpenGL 上下文是单线程的
- 解决方案：工作线程录制 CPU 命令，主线程执行 OpenGL 调用

### 实现模式

```cpp
// 使用 std::async 并行录制图层
auto staticFuture = std::async(std::launch::async, [&]() {
    updateStaticLayer();
    return staticLayer_.surface->takeRecording();
});

auto waveformFuture = std::async(std::launch::async, [&]() {
    updateWaveformLayer();
    return waveformLayer_.surface->takeRecording();
});

auto overlayFuture = std::async(std::launch::async, [&]() {
    updateOverlayLayer();
    return overlayLayer_.surface->takeRecording();
});

// 主线程等待并提交
target->submit(*staticFuture.get());
target->submit(*waveformFuture.get());
target->submit(*overlayFuture.get());
```

### 注意事项

- 需要线程安全的 `GlyphCache`（添加互斥锁或线程本地缓存）
- 当前架构已经为此做好准备（每层独立的 Surface 和 Canvas）

### 实现建议

- **Category**: `ultrabrain`
- **Skills**: 无需特殊技能
- **预估工时**: 8-12 小时
- **依赖**: Phase 1-4 完成后评估是否需要

---

## 实现顺序与依赖

```
Phase 1: 文本批处理
    │
    ├──► Phase 2: DrawOp 排序 ──► Phase 4: 内存优化
    │         │
    │         ▼
    └──► Phase 3: 状态缓存 (并行)
                    │
                    ▼
            Phase 5: 多线程录制 (未来)
```

**推荐顺序**：
1. **Phase 1**（文本批处理）- 最高影响，最低风险
2. **Phase 3**（状态缓存）- 独立，快速收益
3. **Phase 2**（DrawOp 排序）- 基于 Phase 1
4. **Phase 4**（内存优化）- 较低优先级，较高复杂度
5. **Phase 5**（多线程）- 评估是否需要

---

## 验证策略

### 每阶段测试

| 阶段 | 单元测试 | 视觉测试 | 性能测试 |
|------|---------|---------|---------|
| 1 | 文本四边形生成 | 渲染示例 VCD | 绘制调用计数 |
| 2 | SortKey 编码、稳定排序 | 渲染示例 VCD | 状态切换计数 |
| 3 | 缓存命中/未命中跟踪 | 渲染示例 VCD | GL 调用计数 |
| 4 | Arena 分配、引用 | 渲染示例 VCD | 内存使用、缓存未命中 |

### 性能测量

```bash
# 带性能分析构建
cmake -DCMAKE_BUILD_TYPE=RelWithDebInfo ..

# 测量绘制调用（需要 apitrace）
apitrace trace ./waveform_example test.vcd
apitrace dump trace.trace | grep -c glDrawArrays

# 测量缓存性能
perf stat -e cache-misses,cache-references ./waveform_example test.vcd

# 测量帧时间
# 在 paint() 方法中添加计时插桩
```

---

## 预期收益

| 优化 | 预期改进 |
|------|---------|
| 文本批处理 | 绘制调用减少 10-50 倍 |
| DrawOp 排序 | 状态切换减少 50%+ |
| 状态缓存 | GL API 调用减少 50%+ |
| 内存优化 | DrawOp 大小减少 60%，零堆分配 |
| 多线程录制 | 多核 CPU 上录制时间减少 2-3 倍 |

---

## 文件变更清单

### Phase 1
- `glx/gl_context.hpp` - 添加 `TextVertex`, `textVertices_`
- `glx/gl_context.cpp` - 重构 `applyOp(Text)`, 添加 `flushText()`

### Phase 2
- `include/draw_pass.hpp` - 新文件：`SortKey`, `DrawPass`
- `src/draw_pass.cpp` - 新文件：排序和批处理实现
- `src/surface.cpp` - 修改 `submit()` 使用 `DrawPass`

### Phase 3
- `glx/gl_state_cache.hpp` - 新文件：`GlStateCache`
- `glx/gl_state_cache.cpp` - 新文件：状态缓存实现
- `glx/gl_context.cpp` - 集成 `GlStateCache`

### Phase 4
- `include/recording.hpp` - 重构 `DrawOp` 为 `CompactDrawOp`
- `src/recording.cpp` - 添加 `DrawOpArena`
- `glx/gl_context.cpp` - 更新 `applyOp` 使用新结构

### Phase 5
- `include/waveform_viewer.hpp` - 添加线程池/future
- `src/waveform_viewer.cpp` - 并行化 `updateXxxLayer()`
- `include/glyph_cache.hpp` - 添加线程安全

---

## 待确认问题

1. **优先级**：~~优先帧率（Phase 1-3）还是内存效率（Phase 4）？~~ → **内存效率优先**
2. **文本颜色**：~~相邻文本块是否有不同颜色？~~ → **暂时不需要**，Phase 1 可简化
3. **目标硬件**：~~最低 OpenGL 版本要求？~~ → **兼容 GL 3.3 core**，不使用 GL 4.5 DSA
4. **测试数据**：~~是否有大型 VCD 文件？~~ → **有**

---

## 调整后的实现顺序

基于用户反馈，调整优先级：

```
Phase 4 (内存优化) → Phase 1 (文本批处理) → Phase 3 (状态缓存) → Phase 2 (DrawOp 排序) → Phase 5 (多线程)
```

**理由**：
- 内存效率优先，Phase 4 提前
- Phase 1 简化：不需要顶点颜色，单色文本批处理即可
- Phase 3 保持 GL 3.3 兼容，不使用 DSA

---

## 实现状态

| 阶段 | 状态 | 完成日期 |
|------|------|---------|
| **Phase 4** | ✅ 完成 | 2026-01-27 |
| **Phase 1** | ✅ 完成 | 2026-01-27 |
| **Phase 3** | ✅ 完成 | 2026-01-27 |
| **Phase 2** | ✅ 完成 | 2026-01-27 |
| **Phase 5** | ⏳ 待评估 | - |

### 已实现的优化

1. **Phase 4 - 内存优化**
   - `DrawOpArena` 线性分配器
   - `CompactDrawOp` 28 字节结构（vs 原 72+ 字节）
   - 零堆分配录制

2. **Phase 1 - 文本批处理**
   - `TextVertex` 结构和 `textVertices_` 缓冲
   - `flushText()` 单次绘制所有文本
   - 颜色变化时自动刷新

3. **Phase 3 - 状态缓存**
   - `GlStateCache` 类跟踪 GL 状态
   - 缓存 program/VAO/VBO/uniform/scissor/texture
   - 仅在状态变化时调用 GL API

4. **Phase 2 - DrawPass 排序**
   - `SortKey` 64 位键编码
   - `DrawPass` 类实现排序逻辑
   - 基础设施就绪，可选启用
