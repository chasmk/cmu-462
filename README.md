# cmu-462

15-462作业

[TOC]

## 1  Draw SVG

### Task 1: Hardware Renderer

使用opengl库光栅化图形

### Task 2 : Warm Up: Drawing Lines

在屏幕上画一条线，给出的参数如下：

```c++
rasterize_line(float x0, float y0,//点1
               float x1, float y1,//点2
               Color color)
```

直接的想法是算出斜率，然后从点1到点2每走一步画一个点，但这个方法在斜率大于1是会出现断线的情况，如下所示：

![image-20231206205631047](.\assets\image-20231206205631047.png)

所以需要根据斜率调整绘制的步长，而且要保证x和y同时到达目标点，最终效果如下：

![image-20231206205808242](.\assets\image-20231206205808242.png)
