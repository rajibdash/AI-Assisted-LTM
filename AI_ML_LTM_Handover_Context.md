# AI/ML-Enhanced L1/L2 Triggered Mobility (LTM) Context

This technical document outlines the integration of **AI/ML models** with **Layer 1/Layer 2 Triggered Mobility (LTM)** as studied under **3GPP TR 38.745 (Release 20)**. It details the operational impacts on the MAC and UPC schedulers, alongside performance gains in Handover Interruption Time (HOIT).

---

## 1. Impact on the MAC Scheduler
The **MAC Scheduler** handles physical layer time-frequency resource allocation for active User Equipments (UEs). In standard Release 18 LTM, cell switches rely on a lightweight **MAC Control Element (MAC CE)** command rather than heavy Layer 3 RRC Reconfiguration messages. 

When AI/ML models are integrated (per TR 38.745), the MAC Scheduler undergoes key operational optimizations:
*   **Predictive Buffer & Resource Allocation:** By forecasting the UE’s multi-hop trajectory, the target gNB's MAC Scheduler pre-allocates dedicated scheduling blocks and prioritizes the incoming UE *before* the actual cell switch request occurs.
*   **Optimal MAC CE Scheduling:** AI engines filter out temporary signal drops caused by fast-fading anomalies. This ensures the source MAC scheduler sends the LTM cell-switch MAC CE at the exact optimal window, preventing command loss during channel degradation.
*   **Dynamic Beam/CSI-RS Preparation:** AI models assist in predicting the best candidate beam configuration. The MAC scheduler maps the exact Transmission Configuration Indicator (TCI) state immediately upon the cell switch, entirely bypassing scheduling dead-time for beam sweeping.

---

## 2. Impact on the UPC Scheduler (User Plane Control)
The **User Plane Control (UPC) Scheduler** manages packet classification, buffering, and Quality of Service (QoS) guarantees across split-architectures (gNB-CU and gNB-DU).
*   **Proactive Packet Bi-Casting/Forwarding:** Using the AI-predicted target candidate cell, the UPC scheduler initiates selective packet forwarding or dual-connectivity bi-casting in advance. Data resides at the target buffer prior to the L1/L2 switch execution, eliminating traditional buffering queues.
*   **Minimized PDCP/RLC Data Recovery:** A highly synchronized, predictive switch leads to fewer broken user plane sessions. The UPC scheduler experiences a major reduction in Packet Data Convergence Protocol (PDCP) data recovery overhead and Radio Link Control (RLC) retransmissions.

---

## 3. Handover Interruption Time (HIT) Gains
Integrating AI/ML predictive engines eliminates the standard Time-To-Trigger (TTT) window by forecasting stable measurement events before they occur. 

| Mobility Protocol | Handover Interruption Time (HIT) | Core Driving Mechanism |
| :--- | :--- | :--- |
| **Legacy L3 Handover** | **50ms – 90ms** | Reactive L3 RRC Signaling, measurement reports, full protocol stack reset, and post-switch Random Access. |
| **Standard L1/L2 LTM** | **20ms – 30ms** | Target cells are preconfigured. Switching relies on fast lower-layer MAC CE signaling with early DL/UL synchronization. |
| **AI/ML-Enhanced LTM** | **~0ms – Sub-10ms** | AI models bypass the TTT window. Proactive target preparation ensures near-instantaneous MAC-CE execution and seamless beam alignment. |

---

## 4. Key AI Study Areas (3GPP TR 38.745)
*   **Multi-hop UE Trajectory across gNBs:** Prepares multiple target cells sequentially along a predicted movement path.
*   **Intra-CU LTM:** Optimizes L1/L2 Triggered Mobility within the same Central Unit.
*   **Inter-CU LTM:** Manages predictive context transfers and candidate cell preparation across distinct Central Units.
