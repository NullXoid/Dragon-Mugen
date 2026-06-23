; A.Ben command file.
; Original Dragon MUGEN benchmark foundation.
; Buttons: x/y/z = punches, a/b/c = kicks, s = ST / taunt.

;-| Super Motions |--------------------------------------------------------
[Command]
name = "full_charge_rush"
command = ~D, DF, F, D, DF, F, z
time = 24

;-| Special Motions |------------------------------------------------------
[Command]
name = "boost_shot_x"
command = ~D, DF, F, x
time = 15

[Command]
name = "boost_shot_y"
command = ~D, DF, F, y
time = 15

[Command]
name = "boost_shot_z"
command = ~D, DF, F, z
time = 15

[Command]
name = "rising_boost_x"
command = ~F, D, DF, x
time = 15

[Command]
name = "rising_boost_y"
command = ~F, D, DF, y
time = 15

[Command]
name = "rising_boost_z"
command = ~F, D, DF, z
time = 15

;-| Double Tap |-----------------------------------------------------------
[Command]
name = "FF"
command = F, F
time = 10

[Command]
name = "BB"
command = B, B
time = 10

;-| Required Recovery |----------------------------------------------------
[Command]
name = "recovery"
command = x+y
time = 1

;-| Single Button |--------------------------------------------------------
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

;-| Hold Dir |-------------------------------------------------------------
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

;---------------------------------------------------------------------------
; State entry
;---------------------------------------------------------------------------
[Statedef -1]

[State -1, Full Charge Rush]
type = ChangeState
value = 3000
triggerall = command = "full_charge_rush"
triggerall = power >= 1000
triggerall = statetype != A
trigger1 = ctrl

[State -1, Rising Boost Strong]
type = ChangeState
value = 1120
triggerall = command = "rising_boost_z"
triggerall = statetype != A
trigger1 = ctrl

[State -1, Rising Boost Medium]
type = ChangeState
value = 1110
triggerall = command = "rising_boost_y"
triggerall = statetype != A
trigger1 = ctrl

[State -1, Rising Boost Light]
type = ChangeState
value = 1100
triggerall = command = "rising_boost_x"
triggerall = statetype != A
trigger1 = ctrl

[State -1, Boost Shot Strong]
type = ChangeState
value = 1020
triggerall = command = "boost_shot_z"
triggerall = statetype != A
trigger1 = ctrl

[State -1, Boost Shot Medium]
type = ChangeState
value = 1010
triggerall = command = "boost_shot_y"
triggerall = statetype != A
trigger1 = ctrl

[State -1, Boost Shot Light]
type = ChangeState
value = 1000
triggerall = command = "boost_shot_x"
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
