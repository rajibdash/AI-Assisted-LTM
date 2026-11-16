# MediaTek UE Issues in Rel-18 LTM IoDT Testing

## Scope

This note addresses the request to collect MediaTek UE issues observed during IoDT testing in Ericsson's lab for 3GPP Release 18 L1/L2 Triggered Mobility (LTM), limited to FR1 and the intra-gNB/CU stage.

## Finding

**No individual MediaTek UE issues for this test scope are documented in the available source.** The referenced [MediaTek Rel-18 LTM article](../standards/references/MTK_Rel18_LTM.pdf) does not identify an Ericsson-lab IoDT campaign, FR1 test conditions, intra-gNB/CU test-stage results, UE issue IDs, observed failures, reproduction steps, or mitigations.

This is an evidence gap, **not** evidence that testing had zero issues. The source does not support a factual list of UE issues, and no issue details have been inferred or invented here.

## What the source does establish

- It describes Release 18 LTM as covering intra-gNB mobility (article page 5).
- It describes the LTM preparation, early-synchronization, and execution phases (article pages 3–4).
- It reports MediaTek M90 LTM field-trial results on commercial networks, with interruption times in the mid-30 ms range compared with the low-50 ms range for conventional handover (article page 4).
- It mentions Ericsson, AT&T, and MediaTek's in-field trial on AT&T's commercial network using Ericsson RAN (article page 6), and an earlier Ericsson, KDDI, and MediaTek in-field demonstration (article page 6).

These statements are not identified as FR1 IoDT testing in Ericsson's lab and contain no per-UE issue descriptions. The article's general treatment of Release 18 intra-gNB mobility also does not establish a specific CU-stage test result.

## Issue inventory

| Test scope | Documented MediaTek UE issues in the source | Evidence status |
| --- | --- | --- |
| FR1, intra-gNB/CU, Ericsson-lab IoDT | None listed | Not reported in the cited article; test logs or issue tracker are needed |

## Information needed to complete the inventory

To capture actual observed issues, provide the Ericsson-lab IoDT test report, UE/modem logs, or issue-tracker export for this scope. For each confirmed issue, record its issue ID, UE/modem and software build, FR1 band and carrier configuration, intra-gNB/CU test setup, steps to reproduce, expected and actual behavior, impact, frequency, evidence/log references, workaround, and resolution status.

## Source

MediaTek, *Release 18 L1/L2 Triggered Mobility (LTM): A New Standard for Faster Cellular Handover*, repository copy: [`standards/references/MTK_Rel18_LTM.pdf`](../standards/references/MTK_Rel18_LTM.pdf), especially pages 3–6.
