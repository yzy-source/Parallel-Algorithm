并行Dijstra算法
====
### 场景
边缘计算场景下，在边缘层的边缘服务器A可能需要与在同一网络区域的边缘服务器B进行通信，但是他们之间互不相连，所以需要通过其他服务器进行消息转发。整个网络区域不同服务器之前的通信代价不同，如何在A、B之间选择代价最小的通信路径？

### 问题定义
此问题映射成一个有权图的单源最短路径问题，典型的算法是Dijkstra算法。另外，如果这是一个大的网路区域，可能存在多个网络节点，边缘服务器的计算任务量可能会很大，于是可以通过其周围的几个边缘节点进行并行计算，分担其计算量。
