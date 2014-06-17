.. _pdaemon-iredir:
.. _pdaemon-perf-iredir:
.. _pdaemon-io-iredir:
.. _pdaemon-io-iredir-err:
.. _pdaemon-io-iredir-timeout:
.. _pdaemon-intr-iredir-pmc:
.. _pdaemon-subintr-iredir-err:
.. _pdaemon-subintr-host-irq:

=========================
PMC interrupt redirection
=========================

One of PDAEMON powers is redirecting the PMC INTR_HOST interrupt to itself.
The redirection hw may be in one of two states:

- HOST: PMC INTR_HOST output connected to PCI interrupt line [ORed with PMC
  INTR_NRHOST output], PDAEMON falcon interrupt #15 disconnected and forced to 0
- DAEMON: PMC INTR_HOST output connected to PDAEMON falcon interrupt #15
  [IREDIR_PMC], PCI interrupt line connected to INTR_NRHOST output only

In addition, there's a capability enabling host to send "please turn redirect
status back to HOST" interrupt with a timeout mechanism that will execute
the request in hardware if the PDAEMON fails to respond to the interrupt in
a given time.

Note that, as a side effect of having this circuitry, PMC INTR_HOST line will
be delivered nowhere [falcon interrupt line #15 will be 0, PCI interrupt line
will be connected to INTR_NRHOST only] whenever the IREDIR circuitry is in
reset state, due to either whole PDAEMON reset through PMC.ENABLE /
PDAEMON_ENABLE or DAEMON circuitry reset via SUBENGINE_RESET with DAEMON
set in the reset mask.

The redirection state may be read at:

MMIO 0x690 / I[0x1a400]: IREDIR_STATUS
  Read-only. Reads as 0 if redirect hw is in HOST state, 1 if it's in DAEMON
  state.

The redirection state may be controlled by:

MMIO 0x68c / I[0x1a300]: IREDIR_TRIGGER
  This register is write-only.

  - bit 0: HOST_REQ - when written as 1, sends the "request redirect state change
    to HOST" interrupt, setting SUBINTR bit #6 [IREDIR_HOST_REQ] to 1 and
    starting the timeout, if enabled. When written as 1 while redirect hw is
    already in HOST state, will just cause HOST_REQ_REDUNDANT error instead.
  - bit 4: DAEMON - when written as 1, sets the redirect hw state to DAEMON.
    If it was set to DAEMON already, causes DAEMON_REDUNDANT error.
  - bit 12: HOST - when written as 1, sets the redirect hw state to HOST. If it
    was set to HOST already, causes HOST_REDUNDANT error. Does *not* clear
    IREDIR_HOST_REQ interrupt bit.

  Writing a value with multiple bits set is not a good idea - one of them will
  cause an error.

The IREDIR_HOST_REQ interrupt state should be cleared by writing 1 to the
corresponding SUBINTR bit. Once this is done, the timeout counting stops,
and redirect hw goes to HOST state if it wasn't already.

The IREDIR_HOST_REQ timeout is controlled by the following registers:

MMIO 0x694 / I[0x1a500]: IREDIR_TIMEOUT
  The timeout duration in daemon cycles. Read/write, 32-bit.

MMIO 0x6a4 / I[0x1a900]: IREDIR_TIMEOUT_ENABLE
  The timeout enable. Only bit 0 is valid. When set to 0, timeout mechanism
  is disabled, when set to 1, it's active. Read/write.

When timeout mechanism is enabled and IREDIR_HOST_REQ interupt is triggered,
a hidden counter starts counting down. If IREDIR_TIMEOUT cycles go by without
the interrupt being acked, the redirect hw goes to HOST state, the interrupt
is cleared, and HOST_REQ_TIMEOUT error is triggered.

The redirect hw errors will trigger the IREDIR_ERR interrupt, which is
connected to SUBINTR bit #5. The registers involved are:

MMIO 0x698 / I[0x1a600]: IREDIR_ERR_DETAIL
  Read-only, shows detailed error status. All bits are auto-cleared when
  IREDIR_ERR_INTR is cleared

  - bit 0: HOST_REQ_TIMEOUT - set when the IREDIR_HOST_REQ interrupt times out
  - bit 4: HOST_REQ_REDUNDANT - set when HOST_REQ is poked in IREDIR_TRIGGER
    while the hw is already in HOST state
  - bit 12: DAEMON_REDUNDANT - set when HOST is poked in IREDIR_TRIGGER while the
    hw is already in DAEMON state
  - bit 12: HOST_REDUNDANT - set when HOST is poked in IREDIR_TRIGGER while the
    hw is already in HOST state

MMIO 0x69c / I[0x1a700]: IREDIR_ERR_INTR
  The status register for IREDIR_ERR interrupt. Only bit 0 is valid. Set when
  any of the 4 errors happens, cleared [along with all IREDIR_ERR_DETAIL bits]
  when 1 is written to bit 0. When this and IREDIR_ERR_INTR_EN are both set,
  the IREDIR_ERR [#5] second-level interrupt line to SUBINTR is asserted.

MMIO 0x6a0 / I[0x1a800]: IREDIR_ERR_INTR_EN
  The enable register for IREDIR_ERR interrupt. Only bit 0 is valid.

The interrupt redirection circuitry also exports the following signals to
PCOUNTER:

- IREDIR_STATUS: current redirect hw status, like the IREDIR_STATUS reg.
- IREDIR_HOST_REQ: 1 if the IREDIR_HOST_REQ [SUBINTR #6] interrupt is pending
- IREDIR_TRIGGER_DAEMON: pulses for 1 cycle whenever INTR_TRIGGER.DAEMON is
  written as 1, whether it results in an error or not
- IREDIR_TRIGGER_HOST: pulses for 1 cycle whenever INTR_TRIGGER.HOST is
  written as 1, whether it results in an error or not
- IREDIR_PMC: 1 if the PMC INTR_HOST line is active and directed to DAEMON
  [ie. mirrors falcon interrupt #15 input]
- IREDIR_INTR: 1 if any IREDIR interrupt is active - IREDIR_HOST_REQ,
  IREDIR_ERR, or IREDIR_PMC. IREDIR_ERR does not count if IREDIR_ERR_INTR_EN
  is not set.
