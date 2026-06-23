; I.Chie command file.
; Original Dragon MUGEN benchmark foundation.

[Command]
name = "prism_break_rush"
command = ~D, DF, F, D, DF, F, z
time = 24

[Command]
name = "violet_pulse_x"
command = ~D, DF, F, x
time = 15

[Command]
name = "violet_pulse_y"
command = ~D, DF, F, y
time = 15

[Command]
name = "violet_pulse_z"
command = ~D, DF, F, z
time = 15

[Command]
name = "violet_rise_x"
command = ~F, D, DF, x
time = 15

[Command]
name = "violet_rise_y"
command = ~F, D, DF, y
time = 15

[Command]
name = "violet_rise_z"
command = ~F, D, DF, z
time = 15

[Command]
name = "FF"
command = F, F
time = 10

[Command]
name = "BB"
command = B, B
time = 10

[Command]
name = "recovery"
command = x+y
time = 1

[Command]
name = "x"
command = x
time = 1

[Command]
name = "y"
command = y
time = 1

[Command]
name = "z"
command = z
time = 1

[Command]
name = "a"
command = a
time = 1

[Command]
name = "b"
command = b
time = 1

[Command]
name = "c"
command = c
time = 1

[Command]
name = "start"
command = s
time = 1

[Command]
name = "holdfwd"
command = /$F
time = 1

[Command]
name = "holdback"
command = /$B
time = 1

[Command]
name = "holdup"
command = /$U
time = 1

[Command]
name = "holddown"
command = /$D
time = 1

[Statedef -1]

[State -1, Prism Break Rush]
type = ChangeState
value = 3000
triggerall = command = "prism_break_rush"
triggerall = power >= 1000
triggerall = statetype != A
trigger1 = ctrl

[State -1, Violet Rise Strong]
type = ChangeState
value = 1120
triggerall = command = "violet_rise_z"
triggerall = statetype != A
trigger1 = ctrl

[State -1, Violet Rise Medium]
type = ChangeState
value = 1110
triggerall = command = "violet_rise_y"
triggerall = statetype != A
trigger1 = ctrl

[State -1, Violet Rise Light]
type = ChangeState
value = 1100
triggerall = command = "violet_rise_x"
triggerall = statetype != A
trigger1 = ctrl

[State -1, Violet Pulse Strong]
type = ChangeState
value = 1020
triggerall = command = "violet_pulse_z"
triggerall = statetype != A
trigger1 = ctrl

[State -1, Violet Pulse Medium]
type = ChangeState
value = 1010
triggerall = command = "violet_pulse_y"
triggerall = statetype != A
trigger1 = ctrl

[State -1, Violet Pulse Light]
type = ChangeState
value = 1000
triggerall = command = "violet_pulse_x"
triggerall = statetype != A
trigger1 = ctrl

[State -1, Run Fwd]
type = ChangeState
value = 100
trigger1 = command = "FF"
trigger1 = statetype = S
trigger1 = ctrl

[State -1, Run Back]
type = ChangeState
value = 105
trigger1 = command = "BB"
trigger1 = statetype = S
trigger1 = ctrl

[State -1, ST Taunt]
type = ChangeState
value = 195
triggerall = command = "start"
triggerall = statetype = S
trigger1 = ctrl

[State -1, Stand LP]
type = ChangeState
value = 200
triggerall = command = "x"
triggerall = command != "holddown"
triggerall = statetype = S
trigger1 = ctrl

[State -1, Stand MP]
type = ChangeState
value = 210
triggerall = command = "y"
triggerall = command != "holddown"
triggerall = statetype = S
trigger1 = ctrl

[State -1, Stand SP]
type = ChangeState
value = 220
triggerall = command = "z"
triggerall = command != "holddown"
triggerall = statetype = S
trigger1 = ctrl

[State -1, Stand LK]
type = ChangeState
value = 230
triggerall = command = "a"
triggerall = command != "holddown"
triggerall = statetype = S
trigger1 = ctrl

[State -1, Stand MK]
type = ChangeState
value = 240
triggerall = command = "b"
triggerall = command != "holddown"
triggerall = statetype = S
trigger1 = ctrl

[State -1, Stand SK]
type = ChangeState
value = 250
triggerall = command = "c"
triggerall = command != "holddown"
triggerall = statetype = S
trigger1 = ctrl
