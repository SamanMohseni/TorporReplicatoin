# Torpor Replicatoin
This project is part of the **Low Power Design** course.

Here, we will replicate and validate the paper ["Energy and power awareness in hardware schedulers for energy harvesting IoT SoCs"](https://www.sciencedirect.com/science/article/abs/pii/S0167926018305480#:~:text=To%20use%20harvested%20energy%20efficiently,on%20different%20application%2Dspecific%20parameters.) using a custom system-level behavioral simulator, developed in C++.

## Paper Summary
The paper presents Torpor, a power-aware hardware scheduler that enables IoT nodes to efficiently execute part of their applications using irregular harvesting power, while still guaranteeing their low-power always-on required functionality using a battery. Torpor tries to maximally exploit the harvesting power and minimize the battery usage to prolong the battery's lifespan; this goal is achieved by dynamically scheduling available tasks based on parameters such as the task's power consumption and the harvester's output power. by using these parameters torpor tries to more closely match the load’s power consumption and the harvester's output power by minimizing the time intervals where the harvested power exceeds the energy-driven tasks’ power consumption, this minimizes the chance of energy buffer saturation and as a result, maximizes the harvested energy consumption.

## The system architecture
The torpor architecture consists of two main parts, Torpor-Software-Runtime and Torpor-HW; The software-runtime allows the user to specify tasks and scheduling policy and then abstracts this scheduling information and passes it to the HW. The actual scheduling decision is then done by the control logic of the Torpor-HW, based on the scheduling information, while the processor can go to the Idle state. the hardware continuously monitors the harvesters power $(p_h(t))$ and the buffered energy in the hardware's capacitor $(E_{buff}(t))$ and based on these pieces of information the HW makes a scheduling decision, writes the decision in some register, and interrupts the processor, the software-runtime part then reads the HW registers and executes the scheduled task. The Torpor-HW also provides the required hardware components to supply energy to the load in a controlled manner and the required hardware for the necessary monitoring mentioned. Another important feature that Torpor offers is the automatic task characterization 
that determines the required task parameters values. this is done by initially running all application tasks using an external power source, and estimating the power consumption of the task using some measurements over the voltage levels of a capacitor.

**Please refer to the original paper for more details.**

# Replication

## Overview

To verify the performance and compare the proposed scheduling methods, a high-level system simulator was developed using C++. This simulator replicates the experimental conditions as described in the paper. The structure of the simulator includes a function `Ph(t)` to generate the harvested power at any given moment, and consists of three classes: `Buffer`, `TorporLogic`, and `SoC`.

## System-on-Chip (SoC) Simulation

The `SoC` class is responsible for simulating the execution of tasks. Initially, it takes as input a chain of tasks along with their specifications, such as execution time, average power consumption, and the scheduling policy. The class then orchestrates the running of these tasks in accordance with the scheduler's decisions.

## Buffer Management

The `Buffer` class updates the `$V_{buff}$` value given the harvested power, the system's power consumption, and the time intervals (`deltaT`).

## Scheduler Logic (TorporLogic)

The `TorporLogic` class, upon receiving the voltage thresholds for task execution and mask values, performs the task selection for the next job to be executed.

## Simulator Operation

The functions of these classes are invoked at short time intervals (every `deltaT` seconds), updating their respective statuses. The `SoC` assesses whether the currently running task has finished. If so, it configures `TorporLogic` to select the next task and informs it of the task's power consumption during the interval. `Buffer` updates `$V_{buff}$` using the task's power consumption and the harvested power. `TorporLogic` then evaluates `$V_{buff}$` against the set thresholds and, after necessary comparisons, stores the possibility and identifier of the next task to be executed. This information is used by the `SoC` to start the execution of the next task.

## Validation and Results

Through various simulations, results that are largely consistent with the expected outcomes were obtained.

**Our Energy-Oriented Results for $V_{buff}$ and Task executions** (Please refer to the original paper for comparison)
![image](https://github.com/SamanMohseni/TorporReplicatoin/assets/51726090/5239eb0e-9983-46a0-a00a-f617c8ef38ff)

**Our Power-Oriented Results for $V_{buff}$ and Task executions** (Please refer to the original paper for comparison)
![image](https://github.com/SamanMohseni/TorporReplicatoin/assets/51726090/c650a813-55e8-4dd3-a9f2-1267171b607e)

### Summary of our results in comparison to the original results
**Energy-oriented results comparison**
| |  ours   |  paper |
|-|-|-|
|$P_h[mW]$ | 1.27 | 1.34 |
|exec/min| 8.8 | 8.5 |

**Power-oriented results comparison**
| |  ours   |  paper |
|-|-|-|
|$P_h[mW]$ | 1.92 | 1.97 |
|exec/min| 13.1 | 13.2 |

This validates the effectiveness of the Torpor scheduler in utilizing harvested power efficiently and executing tasks at appropriate times, thereby minimizing battery consumption.

## Conclusion

The Torpor power-aware scheduler was examined, and its performance was validated wherever possible. By timely utilizing harvested power and scheduling appropriate tasks, the scheduler aims to improve the efficiency of energy use, thereby minimizing battery consumption. The hardware is designed to have negligible energy consumption when integrated into an SoC.
