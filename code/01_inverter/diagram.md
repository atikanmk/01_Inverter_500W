# Inverter Data Flow Diagram

เอกสารนี้สรุปการส่งผ่านตัวแปร/ข้อมูล 2 ระดับ:
- ระหว่าง module
- ภายใน module (ระหว่าง function)

## 1) Module-Level Variable/Data Flow

```mermaid
flowchart LR
  MAIN[main.c]
  HALL[hall.c]
  TASK[task.c]
  FOC[foc.c]

  HALL -->|Hall_GetPattern() -> hall_pattern| MAIN
  HALL -->|hall_ang_est() -> g_task_hall_angle_est| TASK
  HALL -->|hall_ang_est() -> return angle| FOC

  HALL -->|extern globals (status/angle)
  g_hall1_state,g_hall2_state,g_hall3_state,
  g_hall_state,g_hall_angle_deg,
  hall_est_curr_ang,hall_est_ang_ref,hall_est_ang_next| MAIN
  HALL -->|extern globals (same set)| TASK
  HALL -->|extern globals (same set)| FOC

  TASK -->|extern global
  g_task_hall_angle_est| MAIN
```

หมายเหตุ:
- ในโค้ดปัจจุบัน ตัวแปร extern หลายตัวถูกประกาศเพื่อ monitor/state sharing แต่เส้นทางที่ใช้งานจริงเด่นๆ คือค่าที่คืนจาก Hall_GetPattern() และ hall_ang_est().

## 2) Intra-Module Flow (hall.c)

```mermaid
flowchart TD
  EXTI1[EXTI1_IRQHandler]
  EXTI2[EXTI2_IRQHandler]
  EXTI3[EXTI3_IRQHandler]
  HANDLE[hall_handle_exti_line(line, channel)]
  REFRESH[hall_refresh_all_levels()]
  EST_IRQ[hall_est_reset_on_interrupt()]
  ONEDGE[Hall_OnEdge(channel, level)]

  EXTI1 --> HANDLE
  EXTI2 --> HANDLE
  EXTI3 --> HANDLE

  HANDLE -->|writes hall_last_irq_cycle[channel]| HANDLE
  HANDLE --> REFRESH
  REFRESH -->|writes hall_level[0..2]| REFRESH
  REFRESH -->|writes g_hall1_state,g_hall2_state,g_hall3_state| REFRESH
  REFRESH -->|writes g_hall_state,g_hall_angle_deg| REFRESH

  HANDLE --> EST_IRQ
  EST_IRQ -->|reads g_hall_angle_deg| EST_IRQ
  EST_IRQ -->|writes hall_est_last_cycle,hall_est_prev_sector,hall_est_dir| EST_IRQ
  EST_IRQ -->|writes hall_est_ang_ref,hall_est_ang_next,hall_est_curr_ang| EST_IRQ

  HANDLE -->|increments hall_edge_count[channel]| HANDLE
  HANDLE -->|passes hall_level[channel]| ONEDGE
```

```mermaid
flowchart TD
  ANGEST[hall_ang_est()]
  TIMEOUT[hall_est_is_timeout(now_cycle)]
  FORCE[hall_est_force_reset(now_cycle)]

  ANGEST -->|now_cycle = DWT_CYCCNT| ANGEST
  ANGEST --> TIMEOUT
  TIMEOUT -->|timeout| FORCE

  ANGEST -->|reads hall_est_avg_sector_cycles,hall_est_dir| ANGEST
  ANGEST -->|reads hall_est_last_cycle,hall_est_ang_ref,hall_est_ang_next| ANGEST
  ANGEST -->|writes hall_est_curr_ang| ANGEST
  ANGEST -->|returns hall_est_curr_ang| RET[(return)]
```

## 3) Intra-Module Flow (task.c)

```mermaid
flowchart TD
  IRQ[TIM2_IRQHandler]
  ON100[Task_On100us()]
  EST[hall_ang_est()]

  IRQ -->|g_task_tick_100us++| IRQ
  IRQ --> ON100
  ON100 --> EST
  ON100 -->|writes g_task_hall_angle_est| ON100
```

## 4) Intra-Module Flow (main.c)

```mermaid
flowchart TD
  LOOP[while(1)]
  PATT[Hall_GetPattern()]

  LOOP -->|cnt++| LOOP
  LOOP --> PATT
  PATT -->|returns -> hall_pattern| LOOP
  LOOP -->|GPIOA_IDR bit1 -> pa1_poll| LOOP
  LOOP -->|GPIOA_IDR bit2 -> pa2_poll| LOOP
  LOOP -->|GPIOA_IDR bit3 -> pa3_poll| LOOP
  LOOP -->|compose -> pa123_poll_pattern| LOOP
```

## 5) Intra-Module Flow (foc.c)

```mermaid
flowchart LR
  AE[angle_estimate()]
  HAE[hall_ang_est()]
  AE --> HAE
  HAE -->|return uint16_t angle| AE
```
