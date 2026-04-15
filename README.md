# CUBOT - 基于智能手机视觉的低成本魔方还原机器人

CUBOT 是一个融合了计算机视觉（OpenCV）、嵌入式控制（Arduino）与机械设计的开源项目。它旨在利用低成本的通用零件（如舵机和智能手机），构建一套完整的自动魔方还原系统。

## 🌟 项目亮点
- [cite_start]**低成本设计**：使用 MG996R 舵机与 3D 打印件，无需昂贵的工业机械臂 [cite: 5, 56]。
- [cite_start]**智能手机视觉**：无需专用摄像头，利用手机高画质镜头进行 6 面色彩采集 [cite: 6, 9]。
- [cite_start]**高效算法**：集成 Kociemba 还原算法，自动计算最优解法路径 [cite: 23, 74]。
- [cite_start]**模块化架构**：软硬件解耦，PC 端处理大脑决策，Arduino 处理肢体动作 [cite: 20, 61]。

## 🛠️ 硬件清单
| 零件名称 | 数量 | 用途 |
| :--- | :--- | :--- |
| Arduino Uno | 1 | [cite_start]核心控制逻辑 [cite: 56] |
| MG996R 舵机 | 8 | [cite_start]4个负责旋转盘面，4个负责抓手开合 [cite: 25, 56] |
| PCA9685 驱动板 | 1 | [cite_start]16通道 PWM 舵机控制 [cite: 24, 56] |
| 红外测距传感器 | 1 | [cite_start]检测魔方放入状态 [cite: 185] |
| 3D 打印件 | 一套 | [cite_start]V型抓手与支撑结构 [cite: 56] |
| 智能手机 | 1 | [cite_start]图像采集输入 [cite: 6, 56] |

## 📂 仓库结构
- **Hardware/**: 包含 V 型抓手的 STL 3D 打印模型。
- **Software/Arduino/**: Arduino 源代码，负责舵机 90 度精准旋转与状态机管理。
- **Software/Python/**: Python 源代码，包含颜色识别逻辑与 Kociemba 算法接口。
- **Docs/**: 项目详细技术报告 (PDF版)。

## 🚀 快速开始
1. [cite_start]**硬件组装**：参照 3D 模型打印零件，并按 `CUBOT_Controller.ino` 中的引脚定义连接舵机 [cite: 93, 161]。
2. **烧录代码**：将 Arduino 代码烧录至 Uno 板。
3. **环境配置**：PC 端安装依赖 `pip install opencv-python kociemba pyserial`。
4. [cite_start]**运行**：连接手机至 PC，启动 `vision_solver.py` [cite: 30]。

## 📈 项目结论
[cite_start]本项目成功验证了通过 8 个低成本舵机实现魔方自动还原的可行性 [cite: 66][cite_start]。虽然在复杂光照环境下颜色识别仍有优化空间，但整体系统已经能完成从“扫描”到“还原”的全流程闭环 [cite: 68]。
