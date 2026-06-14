;-| AI |------------------------------------------------------
[Command]
name = "CPU1"
command = U, D, F
time = 1

[Command]
name = "CPU2"
command = U, B, F
time = 1

[Command]
name = "CPU3"
command = U, D, D
time = 1

[Command]
name = "CPU4"
command = F, B, U
time = 1

[Command]
name = "CPU5"
command = U, F, U, B
time = 1

[Command]
name = "CPU6"
command = U, D, B
time = 1

[Command]
name = "CPU7"
command = F, F, B
time = 1

[Command]
name = "CPU8"
command = U, D, U
time = 1

[Command]
name = "CPU9"
command = F, B, B
time = 1

[Command]
name = "CPU10"
command = F, F, B, B
time = 1

[Command]
name = "CPU11"
command = U, U, F
time = 1

[Command]
name = "CPU12"
command = U, B, B
time = 1

[Command]
name = "CPU13"
command = U, B, F, F
time = 1

[Command]
name = "CPU14"
command = U, F, B, U
time = 1

[Command]
name = "CPU15"
command = U, B, F, U
time = 1

[Command]
name = "CPU16"
command = U, B, B, B
time = 1

[Command]
name = "CPU17"
command = U, D, B, F
time = 1

[Command]
name = "CPU18"
command = U, D, B, D
time = 1

[Command]
name = "CPU19"
command = U, D, F, U
time = 1

[Command]
name = "CPU20"
command = U, D, U, B
time = 1

[Command]
name = "CPU21"
command = U, D, F, F
time = 1

[Command]
name = "CPU22"
command = F, F, F, F
time = 1

[Command]
name = "CPU23"
command = U, U, U, D
time = 1

[Command]
name = "CPU24"
command = B, B, B
time = 1

[Command]
name = "CPU25"
command = D, D, D, D
time = 1

;-| Button Remapping |-----------------------------------------------------
[Remap]
x = x
y = y
z = z
a = a
b = b
c = c
s = s
;-| Default Values |-------------------------------------------------------
[Defaults]
command.time = 15
command.buffer.time = 1

;-| Super Motions |--------------------------------------------------------

[Command]
name="Super1"
command=D, DF, F, x+y
time=48

[Command]
name="Super1"
command=D, DF, F, y+z
time=48

[Command]
name="Super1"
command=D, DF, F, x+z
time=48


[Command]
name="BoleaKick"
command=D,B,D,B,a
time=48

[Command]
name="BoleaKick"
command=D,B,D,B,b
time=48


;-| Special Motions |------------------------------------------------------

[command]
name = "RisingUppercut1"
command = ~D, DF, F, x

[command]
name = "RisingUppercut1"
command = ~D, DF, F, y

[command]
name = "RisingUppercut2"
command = ~D, DF, F, z

[command]
name = "RisingSun"
command = ~D, DB, B, a

[command]
name = "RisingSun"
command = ~D, DB, B, b

[command]
name = "RisingSun"
command = ~D, DB, B, c

[command]
name = "HellLancer"
command = ~D, DF, F, a

[command]
name = "HellLancer"
command = ~D, DF, F, b

[command]
name = "HellLancer"
command = ~D, DF, F, c

[command]
name = "Fist"
command = ~D, DB, B, x

[command]
name = "Fist"
command = ~D, DB, B, y

[command]
name = "Fist"
command = ~D, DB, B, z


[Command]
name = "412p" ;Zero Counter
command = ~B, DB, D, x
time = 16

[Command]
name = "412p" ;Zero Counter
command = ~B, DB, D, y
time = 16

[Command]
name = "412p" ;Zero Counter
command = ~B, DB, D, z
time = 16

[Command]
name = "412p" ;Zero Counter
command = ~B, DB, D, ~x
time = 16

[Command]
name = "412p" ;Zero Counter
command = ~B, DB, D, ~y
time = 16

[Command]
name = "412p" ;Zero Counter
command = ~B, DB, D, ~z
time = 16

[Command]
name = "412k" ;Zero Counter
command = ~B, DB, D, a
time = 16

[Command]
name = "412k" ;Zero Counter
command = ~B, DB, D, b
time = 16

[Command]
name=  "412k" ;Zero Counter
command = ~B, DB, D, c
time = 16

[Command]
name = "412k" ;Zero Counter
command = ~B, DB, D, ~a
time = 16

[Command]
name = "412k" ;Zero Counter
command = ~B, DB, D, ~b
time = 16

[Command]
name = "412k" ;Zero Counter
command = ~B, DB, D, ~c
time = 16

;-| Double Tap |-----------------------------------------------------------
[Command]
name = "FF"     ;Required (do not remove)
command = F, F
time = 10

[Command]
name = "BB"     ;Required (do not remove)
command = B, B
time = 10

;-| 2/3 Button Combination |-----------------------------------------------
[Command]
name = "recovery";Required (do not remove)
command = x+y
time = 1

[Command]
name = "recovery"
command = y+z
time = 1

[Command]
name = "recovery"
command = x+z
time = 1

[Command]
name = "recovery"
command = a+x
time = 1

[Command]
name = "pp"
command = x+y
time = 1

[Command]
name = "pp"
command = x+z
time = 1

[Command]
name = "pp"
command = y+z
time = 1

[Command]
name = "kk"
command = a+b
time = 1

[Command]
name = "kk"
command = a+c
time = 1

[Command]
name = "kk"
command = b+c
time = 1

[Command]
name = "a+x"
command = a+x
time=1

[Command]
name = "b+y"
command = b+y
time = 1

[Command]
name = "c+z"
command = c+z
time = 1

;-| Dir + Button |---------------------------------------------------------
[Command]
name = "back_x"
command = /$B,x
time = 1

[Command]
name = "back_y"
command = /$B,y
time = 1

[Command]
name = "back_z"
command = /$B,z
time = 1

[Command]
name = "down_x"
command = /$D,x
time = 1

[Command]
name = "down_y"
command = /$D,y
time = 1

[Command]
name = "down_z"
command = /$D,z
time = 1

[Command]
name = "fwd_x"
command = /$F,x
time = 1

[Command]
name = "fwd_y"
command = /$F,y
time = 1

[Command]
name = "fwd_z"
command = /$F,z
time = 1

[Command]
name = "up_x"
command = /$U,x
time = 1

[Command]
name = "up_y"
command = /$U,y
time = 1

[Command]
name = "up_z"
command = /$U,z
time = 1

[Command]
name = "back_a"
command = /$B,a
time = 1

[Command]
name = "back_b"
command = /$B,b
time = 1

[Command]
name = "back_c"
command = /$B,c
time = 1

[Command]
name = "down_a"
command = /$D,a
time = 1

[Command]
name = "down_b"
command = /$D,b
time = 1

[Command]
name = "down_c"
command = /$D,c
time = 1

[Command]
name = "fwd_a"
command = /$F,a
time = 1

[Command]
name = "fwd_b"
command = /$F,b
time = 1

[Command]
name = "fwd_c"
command = /$F,c
time = 1

[Command]
name = "up_a"
command = /$U,a
time = 1

[Command]
name = "up_b"
command = /$U,b
time = 1

[Command]
name = "up_c"
command = /$U,c
time = 1

;-| Single Button |---------------------------------------------------------
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
name = "s"
command = s
time = 1

;-| Single Dir |------------------------------------------------------------
[Command]
name = "fwd" ;Required (do not remove)
command = $F
time = 1

[Command]
name = "downfwd"
command = $DF
time = 1

[Command]
name = "down" ;Required (do not remove)
command = $D
time = 1

[Command]
name = "downback"
command = $DB
time = 1

[Command]
name = "back" ;Required (do not remove)
command = $B
time = 1

[Command]
name = "upback"
command = $UB
time = 1

[Command]
name = "up" ;Required (do not remove)
command = $U
time = 1

[Command]
name = "upfwd"
command = $UF
time = 1

;-| Hold Button |--------------------------------------------------------------
[Command]
name = "hold_x"
command = /x
time = 1

[Command]
name = "hold_y"
command = /y
time = 1

[Command]
name = "hold_z"
command = /z
time = 1

[Command]
name = "hold_a"
command = /a
time = 1

[Command]
name = "hold_b"
command = /b
time = 1

[Command]
name = "hold_c"
command = /c
time = 1

[Command]
name = "hold_s"
command = /s
time = 1

;-| Hold Dir |--------------------------------------------------------------
[Command]
name = "holdfwd";Required (do not remove)
command = /$F
time = 1

[Command]
name = "holdback";Required (do not remove)
command = /$B
time = 1

[Command]
name = "holdup" ;Required (do not remove)
command = /$U
time = 1

[Command]
name = "holddown";Required (do not remove)
command = /$D
time = 1

[Command]
name = "holdfwd" ;Required (do not remove)
command = /$F
time = 1

[Command]
name = "holddownfwd"
command = /$DF
time = 1

[Command]
name = "holddown" ;Required (do not remove)
command = /$D
time = 1

[Command]
name = "holddownback"
command = /$DB
time = 1

[Command]
name = "holdback" ;Required (do not remove)
command = /$B
time = 1

[Command]
name = "holdupback"
command = /$UB
time = 1

[Command]
name = "holdup" ;Required (do not remove)
command = /$U
time = 1

[Command]
name = "holdupfwd"
command = /$UF
time = 1

;---------------------------------------------------------------------------
;Release Direction
[Command]
name = "rlsfwd"
command = ~$F
time = 1

[Command]
name = "rlsback"
command = ~$B
time = 1

[Command]
name = "rlsup"
command = ~$U
time = 1

[Command]
name = "rlsdown"
command = ~$D
time = 1

;--------------------------------------------------------------------------
;Release Button
[Command]
name = "rlsx"
command = ~x
time = 1

[Command]
name = "rlsy"
command = ~y
time = 1

[Command]
name = "rlsz"
command = ~z
time = 1

[Command]
name = "rlsa"
command = ~a
time = 1

[Command]
name = "rlsb"
command = ~b
time = 1

[Command]
name = "rlsc"
command = ~c
time = 1
;---------------------------------------------------------------------------
;Other
[Command]
name = "highjump"
command = $D, $U
time = 15
;---------------------------------------------------------------------------
[Statedef -1]

[State -1, Tick Fix]
type = CtrlSet
triggerall = !ctrl
trigger1 = (StateNo = 52 || StateNo = 101 || StateNo = 5120) && !AnimTime
trigger2 = (StateNo = [200,499]) && !AnimTime
trigger3 = (StateNo = [760,762]) && !AnimTime
trigger4 = StateNo = 810 && !AnimTime
trigger5 = (StateNo = 5001 || StateNo = 5011 || StateNo = 151 || StateNo = 153) && HitOver
trigger6 = (StateNo = [700,715]) && !AnimTime
value = 1

[State -1, CtrlSet For Helpers];special thanks to 20S
type = CtrlSet
trigger1 = IsHelper
value = 0

[State -1, Hit Count For Helpers];special thanks to 20S
type = ParentVarAdd
trigger1 = IsHelper
trigger1 = MoveHit = 1
trigger1 = !HitPauseTime
trigger1 = !(HitDefAttr = SCA, AT)
var(13) = 1

[State -1, Juggle Count For Helpers];special thanks to 20S
type = ParentVarAdd
trigger1 = IsHelper
trigger1 = MoveHit = 1
trigger1 = !HitPauseTime
trigger1 = !(HitDefAttr = SCA, AT)
var(15) = 1

;[State -1, ProjContact];special thanks to 20S
;type = VarSet
;trigger1 = IsHelper
;trigger1 = MoveContact = 1 && NumTarget
;var(18) = 1

;[State -1, Root ProjContact];special thanks to 20S
;type = ParentVarSet
;trigger1 = IsHelper
;trigger1 = MoveContact = 1 && NumTarget
;var(18) = 1

;[State -1, Grand KF Upper]
;type = ChangeState
;value = 3200
;triggerall = var(59) != 1
;triggerall = command = "Ultra2"
;triggerall = RoundState = 2 && StateType != A
;triggerall = ifelse(var(20) <= 0, power >= 3000, power >= 1000)
;trigger1 = ctrl || StateNo = 40 || StateNo = 52 || (StateNo = [100,101])
;trigger2 = var(6)

[State -1, Grand KF Upper]
type = NULL;ChangeState
value = 3100
triggerall = var(59) != 1
triggerall = command = "BoleaKick"
triggerall = RoundState = 2 && StateType != A
triggerall = ifelse(var(20) <= 0, power >= 2000, power >= 1000)
trigger1 = ctrl || StateNo = 40 || StateNo = 52 || (StateNo = [100,101])
trigger2 = var(6)

[State -1, Kung Fu Barrage]
type = ChangeState
value = 3000
triggerall = var(59) != 1
triggerall = command = "Super1"
triggerall = RoundState = 2 && StateType != A
triggerall = ifelse(var(20) <= 0, power >= 1000, power >= 0)
trigger1 = ctrl || StateNo = 40 || StateNo = 52 || (StateNo = [100,101])
trigger2 = var(6)

;---------------------------------------------------------------------------
[State -1, hadouken]
type = ChangeState
value = 1000
triggerall = var(59) != 1
triggerall = roundstate = 2
triggerall = Command = "RisingUppercut1"
triggerall = RoundState = 2 && StateType != A
trigger1 = ctrl || StateNo = 40 || StateNo = 52 || (StateNo = [100,101])
trigger2 = var(5)

[State -1, hadouken]
type = ChangeState
value = 1010
triggerall = var(59) != 1
triggerall = roundstate = 2
triggerall = Command = "RisingUppercut2"
triggerall = RoundState = 2 && StateType != A
trigger1 = ctrl || StateNo = 40 || StateNo = 52 || (StateNo = [100,101])
trigger2 = var(5)

;---------------------------------------------------------------------------
[State -1, hadouken]
type = ChangeState
value = 1100
triggerall = var(59) != 1
triggerall = roundstate = 2
triggerall = Command = "RisingSun"
triggerall = RoundState = 2 && StateType != A
trigger1 = ctrl || StateNo = 40 || StateNo = 52 || (StateNo = [100,101])
trigger2 = var(5)

;---------------------------------------------------------------------------
[State -1, hadouken]
type = ChangeState
value = 1200
triggerall = var(59) != 1
triggerall = roundstate = 2
triggerall = Command = "HellLancer"
triggerall = RoundState = 2 && StateType != A
trigger1 = ctrl || StateNo = 40 || StateNo = 52 || (StateNo = [100,101])
trigger2 = var(5)

;---------------------------------------------------------------------------
[State -1, hadouken]
type = ChangeState
value = 1300
triggerall = var(59) != 1
triggerall = roundstate = 2
triggerall = Command = "Fist"
triggerall = RoundState = 2 && StateType != A
trigger1 = ctrl || StateNo = 40 || StateNo = 52 || (StateNo = [100,101])
trigger2 = var(5)

;---------------------------------------------------------------------------
[State -1, Power Charge]
type = ChangeState
value = 730
triggerall = var(59) != 1
trigger1 = command = "hold_b" && command = "hold_y"
trigger1 = RoundState = 2 && StateType != A
trigger1 = power < const(data.power) && power < PowerMax && !var(20)
trigger1 = ctrl || (StateNo = [100,101])

[State -1, Run Fwd]
type = ChangeState
value = 100
triggerall = var(59) != 1
trigger1 = command = "FF"
trigger1 = statetype = S
trigger1 = ctrl

[State -1, Run Back]
type = ChangeState
value = 105
triggerall = var(59) != 1
trigger1 = command = "BB"
trigger1 = statetype = S
trigger1 = ctrl

;--------------------------------------------------------------------------
[State -1, Stand Light Punch]
type = ChangeState
value = 200
triggerall = var(59) != 1
triggerall = command = "x"
triggerall = command != "holddown"
trigger1 = statetype != A
trigger1= ctrl || (stateno=[100,101])
trigger2= StateNo = 230 && movecontact

[State -1, Stand Medium Punch]
type = ChangeState
value = 210
triggerall = var(59) != 1
triggerall = command = "y"
triggerall = command != "holddown"
triggerall = StateType != A
trigger1 = ctrl || (StateNo = [100,101])
trigger2= StateNo = 200 && movecontact

[State -1, Stand Medium Punch]
type = ChangeState
value = 220
triggerall = var(59) != 1
triggerall = command = "z"
triggerall = command != "holddown"
triggerall = StateType != A
trigger1 = ctrl || (StateNo = [100,101])
trigger2= StateNo = 210 && movecontact
trigger3= StateNo = 240 && movecontact

[State -1, Stand Light Kick]
type = ChangeState
value = 230
triggerall = var(59) != 1
triggerall = command = "a"
triggerall = command != "holddown"
trigger1 = statetype != A
trigger1= ctrl || (stateno=[100,101])
trigger2= StateNo = 200 && movecontact

[State -1, Stand Medium Kick]
type = ChangeState
value = 240
triggerall = var(59) != 1
triggerall = command = "b"
triggerall = command != "holddown"
trigger1 = statetype != A
trigger1= ctrl || (stateno=[100,101])
trigger2= StateNo = 230 && movecontact

[State -1, Stand Medium Kick]
type = ChangeState
value = 250
triggerall = var(59) != 1
triggerall = command = "c"
triggerall = command != "holddown"
trigger1 = statetype != A
trigger1= ctrl || (stateno=[100,101])
trigger2= StateNo = 230 && movecontact
trigger3= StateNo = 240 && movecontact
trigger4= StateNo = 210 && movecontact

[State -1, Crouching Light Punch]
type = ChangeState
value = 400
triggerall = var(59) != 1
triggerAll = command = "holddown" && command = "x"
triggerAll = StateType != A
trigger1 = ctrl || (StateNo = [100,101])
trigger2= stateno = 400 && movecontact

[State -1, Crouching Light Punch]
type = ChangeState
value = 410
triggerall = var(59) != 1
triggerAll = command = "holddown" && command = "y"
triggerAll = StateType != A
trigger1 = ctrl || (StateNo = [100,101])
trigger2= stateno = 400 && movecontact

[State -1, Crouching Medium Punch]
type = ChangeState
value = 420
triggerall = var(59) != 1
triggerAll = command = "holddown" && command = "z"
triggerAll = StateType != A
trigger1 = ctrl || (StateNo = [100,101])

[State -1, Crouching Light Kick]
type = ChangeState
value = 430
triggerall = var(59) != 1
triggerAll = command = "holddown" && command = "a"
triggerAll = StateType != A
trigger1 = ctrl || (StateNo = [100,101])
trigger2= stateno = 430 && movecontact

[State -1, Crouching Light Kick]
type = ChangeState
value = 440
triggerall = var(59) != 1
triggerAll = command = "holddown" && command = "b"
triggerAll = StateType != A
trigger1 = ctrl || (StateNo = [100,101])
trigger2= stateno = 430 && movecontact

[State -1, Crouching Strong Kick]
type = ChangeState
value = 450
triggerall = var(59) != 1
triggerAll = command = "holddown" && command = "c"
triggerAll = StateType != A
trigger1 = ctrl || (StateNo = [100,101])

[State -1, Jump Light Punch]
type = ChangeState
value = 600
triggerall = var(59) != 1
triggerall = command = "x"
trigger1 = statetype = A
trigger1 = ctrl

[State -1, Jump Light Punch]
type = ChangeState
value = 610
triggerall = var(59) != 1
triggerall = command = "y"
trigger1 = statetype = A
trigger1 = ctrl

[State -1, Jump Strong Punch]
type = ChangeState
value = 620
triggerall = var(59) != 1
triggerall = command = "z"
trigger1 = statetype = A
trigger1 = ctrl

[State -1, Jump Light Kick]
type = ChangeState
value = 630
triggerall = var(59) != 1
triggerall = command = "a"
trigger1 = statetype = A
trigger1 = ctrl

[State -1, Jump Light Kick]
type = ChangeState
value = 640
triggerall = var(59) != 1
triggerall = command = "b"
trigger1 = statetype = A
trigger1 = ctrl

[State -1, Jump Strong Kick]
type = ChangeState
value = 650
triggerall = var(59) != 1
triggerall = command = "c"
trigger1 = statetype = A
trigger1 = ctrl

[State -1, Taunt]
type = ChangeState
value = 195
triggerall = var(59) != 1
triggerall = command = "s"
triggerall = StateType != A
triggerall = StateNo != [200,699]
trigger1 = ctrl || (StateNo = [100,101])
trigger2 = var(5)


;===========================================================================
;=================================<A.I.>====================================
;===========================================================================
[State -1, AI]
type = VarSet
triggerall = var(59) != 1
triggerall = RoundState != 3
trigger1  = command = "CPU1"
trigger2  = command = "CPU2"
trigger3  = command = "CPU3"
trigger4  = command = "CPU4"
trigger5  = command = "CPU5"
trigger6  = command = "CPU6"
trigger7  = command = "CPU7"
trigger8  = command = "CPU8"
trigger9  = command = "CPU9"
trigger10  = command = "CPU10"
trigger11  = command = "CPU11"
trigger12  = command = "CPU12"
trigger13  = command = "CPU13"
trigger14  = command = "CPU14"
trigger15  = command = "CPU15"
trigger16  = command = "CPU16"
trigger17  = command = "CPU17"
trigger18  = command = "CPU18"
trigger19  = command = "CPU19"
trigger20  = command = "CPU20"
trigger21  = command = "CPU21"
trigger22  = command = "CPU22"
trigger23  = command = "CPU23"
trigger24  = command = "CPU24"
trigger25  = command = "CPU25"
var(59) = 1

[State -1, airrecover]
type=changestate
value= ifelse((pos y>=-20), 5200, 5210)
trigger1= var(59)>=1 && numenemy
trigger1= roundstate=2 && alive
trigger1= stateno=5050 && canrecover
trigger1= vel y>=-1 && random<200

[State -1, recoveryroll]
type=changestate
value=5200
trigger1= var(59)>=1 && numenemy
trigger1= roundstate=2 && alive
trigger1= !ctrl
trigger1= (stateno=5040 || stateno=5050) && vel y>=-1 && pos y>-vel y
trigger1= (p2bodydist x=[-10,10]) && random<200

[State -1, goushoryuuken]
type=changestate
value=1200
triggerall= var(59)>=1 && numenemy
triggerall= roundstate=2 && statetype!=A
triggerall= !(enemynear,ctrl) && (enemynear,stateno!=[120,155])
triggerall= (p2statetype!=L || p2stateno=5120) && (p2bodydist x=[0,80]) && (p2dist y=[-120,0])
triggerall= (enemynear,const(size.head.pos.y)<=-40) || (enemynear,statetype=A)
trigger1= ctrl && p2statetype=A && random<ifelse(prevstateno=1200, 333, 200)
trigger2= (stateno=[200,250])
trigger2= movehit && (p2bodydist x=[0,12]) && random<500
trigger4= ctrl && enemynear,movetype=A && (p2bodydist x=[0,40]) && random<500
trigger5= stateno=0 && prevstateno=5120 && time<=1
trigger5= ctrl && (p2bodydist x=[-40,40]) && random<500
trigger6= ctrl && (p2bodydist x=[-30,30])
trigger6= (enemynear,stateno=5120) && (enemynear,animtime=[-6,-3]) && random<250

[State -1, ashurasenkuu]
type=changestate
value=1300
triggerall= var(59)>=1 && numenemy
triggerall= statetype!=A && roundstate=2
triggerall= ctrl
trigger1= enemynear,movetype=A && (p2bodydist x=[-90,90])
trigger1= (enemynear,p2bodydist x>0) && (enemynear,facing!=facing)
trigger1= random<ifelse((enemy,hitdefattr=SC,AT),500,250)
trigger1= var(10):=2

[State -1, roll/dodge]
type=changestate
value=ifelse(random<250,1200,1300)
trigger1= var(59)>=1 && numenemy
trigger1= roundstate=2 && statetype!=A
trigger1= ctrl && random<50
trigger1= (enemynear,movetype=A) && (enemynear,hitdefattr=SCA,AA)
trigger1= (p2bodydist x=[40,120]) && (enemynear,animtime<=-28)

[State -1, tenmashurettou]
type=changestate
value=1000
trigger1= var(59)>=1 && numenemy
trigger1= roundstate=2 && statetype!=A
trigger1= ctrl && (p2bodydist x=[-60,60])
trigger1= enemynear,movetype=A && (enemy,hitdefattr=SCA,AA) && random<200

[State -1, backdash]
type=changestate
value=105
trigger1= var(59)>=1 && numenemy
trigger1= roundstate=2 && statetype=S
trigger1= random < ifelse((enemynear,hitdefattr=SC,AT), 150, 50)
trigger1= ctrl && (stateno!=[100,106]) && (stateno!=[700,701])
trigger1= (enemynear,movetype=A) && backedgedist>=80 && (p2bodydist x=[80,120]) && (enemynear,vel x)

[State -1, Guard]
type=changestate
value=120
trigger1= var(59)>=1 && numenemy
trigger1= roundstate=2 && inguarddist
trigger1= ctrl && (stateno!=[120,155]) && !var(20)
trigger1= ifelse(statetype=A, (var(9)!=2 || stateno=5210), 1)
trigger1= !(enemynear,hitdefattr=SCA,AT) && (enemynear,time<120)
trigger1= statetype!=A || p2statetype=A
trigger1= random<ifelse((p2stateno=[200,699]), 100, ifelse((p2stateno=[1000,2999]), 333, 1000))

[State -1, run/dash]
type=changestate
value=100
trigger1= var(59)>=1 && numenemy
trigger1= statetype=S && roundstate=2
trigger1= ctrl && (stateno!=[100,105])
trigger1= !inguarddist && (p2bodydist x=[60,100]) && random<100

[State -1, Jump]
type=changestate
value=40
trigger1= var(59)>=1 && numenemy
trigger1= roundstate=2 && statetype!=A && ctrl
trigger1= enemynear,movetype=A && p2bodydist x<160 && enemynear,hitdefattr=SC,AT

[State -1, Throw]
type=changestate
value=1000
triggerall= var(59)>=1 && numenemy
triggerall= roundstate=2 && statetype=S
triggerall= p2statetype!=A && p2statetype!=L && p2movetype!=H
triggerall= (p2bodydist x=[0,20]) && p2dist y=0
trigger1= ctrl && random<100
trigger2= ctrl && (p2stateno=[120,140]) && random<750

[State -1, SHP]
type=changestate
value=210
triggerall= var(59)>=1 && numenemy
triggerall= roundstate=2 && statetype=S
triggerall= (p2bodydist x=[0,80]) && (p2dist y=[-50,50]) && p2statetype!=C && p2statetype!=L && !(enemynear,hitfall)
triggerall= (p2stateno!=[120,155]) && p2movetype!=A && !(enemynear,hitfall)
triggerall= (enemynear,const(size.head.pos.y)<=-40) || (enemynear,statetype=A)
trigger1= ctrl && p2bodydist x<25 && random<100
trigger2= (stateno=200 || stateno=210 || stateno=220 || stateno=230 || stateno=240 || stateno=250 || stateno=400 || stateno=430)
trigger2= p2bodydist x<=50 && movehit && random<250

[State -1, SHK]
type=changestate
value=220
triggerall= var(59)>=1 && numenemy
triggerall= roundstate=2 && statetype=S
triggerall= (p2bodydist x=[0,70]) && (p2dist y=[-50,50]) && p2statetype!=C && p2statetype!=L && !(enemynear,hitfall)
triggerall= (p2stateno!=[120,155]) && p2movetype!=A && !(enemynear,hitfall)
triggerall= (enemynear,const(size.head.pos.y)<=-40) || (enemynear,statetype=A)
trigger1= (stateno=200 || stateno=210 || stateno=220 || stateno=230 || stateno=240 || stateno=250 || stateno=400 || stateno=430)
trigger1= p2bodydist x=0 && movehit && random<250

[State -1, SMP2]
type=changestate
value=210
triggerall= var(59)>=1 && numenemy
triggerall= roundstate=2 && statetype=S
triggerall= (p2bodydist x=[0,45]) && (p2dist y=[-50,0]) && p2statetype=C && !(enemynear,hitfall)
triggerall= ((p2stateno!=[120,155]) || p2stateno=131 || p2stateno=152 || p2stateno=153) && p2movetype!=A && !(enemynear,hitfall)
triggerall= (enemynear,const(size.head.pos.y)<=-40) || (enemynear,statetype=A)
trigger1= ctrl && random < 100

[State -1, SMK2]
type=changestate
value=210
triggerall= var(59)>=1 && numenemy
triggerall= roundstate=2 && statetype=S
triggerall= (p2bodydist x=[0,80]) && (p2dist y=[-50,0]) && p2statetype=C && !(enemynear,hitfall)
triggerall= ((p2stateno!=[120,155]) || p2stateno=131 || p2stateno=152 || p2stateno=153) && p2movetype!=A && !(enemynear,hitfall)
triggerall= (enemynear,const(size.head.pos.y)<=-40) || (enemynear,statetype=A)
trigger1= ctrl && random < 50

[State -1, SMP]
type=changestate
value=210
triggerall= var(59)>=1 && numenemy
triggerall= roundstate=2 && statetype=S
triggerall= (p2bodydist x=[0,60]) && (p2dist y=[-50,50]) && p2statetype!=C && p2statetype!=L && !(enemynear,hitfall)
triggerall= (p2stateno!=[120,155]) && p2movetype!=A && !(enemynear,hitfall)
triggerall= (enemynear,const(size.head.pos.y)<=-40) || (enemynear,statetype=A)
trigger1= (stateno=200 || stateno=230 || stateno=240 || stateno=400)
trigger1= p2bodydist x<=16 && movehit && random<250

[State -1, SMK]
type=changestate
value=220
triggerall= var(59)>=1 && numenemy
triggerall= roundstate=2 && statetype=S
triggerall= (p2bodydist x=[0,60]) && (p2dist y=[-50,50]) && p2statetype!=C && p2statetype!=L && !(enemynear,hitfall)
triggerall= (p2stateno!=[120,155]) && p2movetype!=A && !(enemynear,hitfall)
triggerall= (enemynear,const(size.head.pos.y)<=-40) || (enemynear,statetype=A)
trigger1= (stateno=200 || stateno=210 || stateno=220 || stateno=230 || stateno=240 || stateno=250 || stateno=400 || stateno=430)
trigger1= p2bodydist x<=16 && movehit && random<250

[State -1, SLP]
type=changestate
value=200
triggerall= var(59)>=1 && numenemy
triggerall= roundstate=2 && statetype=S && !inguarddist
triggerall= (p2bodydist x=[0,60]) && (p2dist y=[-50,50]) && p2statetype!=C && p2statetype!=L && !(enemynear,hitfall)
triggerall= (p2stateno!=[120,155]) && p2movetype!=A && !(enemynear,hitfall)
triggerall= (enemynear,const(size.head.pos.y)<=-40) || (enemynear,statetype=A)
trigger1= ctrl && random<100
trigger2= (stateno=200 || stateno=230 || stateno=250) && time>=5 && random<50

[State -1, SLK]
type=changestate
value=230
triggerall= var(59)>=1 && numenemy
triggerall= roundstate=2 && statetype=S && !inguarddist
triggerall= (p2bodydist x=[0,60]) && (p2dist y=[-50,50]) && p2statetype!=C && p2statetype!=L && !(enemynear,hitfall)
triggerall= (p2stateno!=[120,155]) && p2movetype!=A && !(enemynear,hitfall)
trigger1= (stateno=200 || stateno=230)
trigger1= p2bodydist x<=4 && (movehit=[1,4]) && random<250

[State -1, CHP]
type=changestate
value=420
triggerall= var(59)>=1 && numenemy
triggerall= roundstate=2 && statetype=S && !inguarddist
triggerall= (p2bodydist x=[0,40]) && (p2dist y=[-50,50]) && p2statetype!=L && !(enemynear,hitfall)
triggerall= (p2stateno!=[120,155]) && p2movetype!=A && !(enemynear,hitfall)
triggerall= (enemynear,const(size.head.pos.y)<=-40) || (enemynear,statetype=A)
trigger1= (stateno=200 || stateno=210 || stateno=220 || stateno=230 || stateno=240 || stateno=250 || stateno=400 || stateno=430)
trigger1= p2bodydist x<=4 && (movehit=[1,4]) && random<250

[State -1, CHK]
type=changestate
value=450
triggerall= var(59)>=1 && numenemy
triggerall= roundstate=2 && statetype=S && !inguarddist
triggerall= (p2bodydist x=[0,60]) && (p2dist y=[-50,50]) && p2statetype!=A && p2stateno!=5120
triggerall= ((p2stateno!=[120,155]) || p2stateno=130 || p2stateno=150 || p2stateno=151) && p2movetype!=A
trigger1= (stateno=200 || stateno=210 || stateno=220 || stateno=230 || stateno=240 || stateno=250 || stateno=400 || stateno=430)
trigger1= p2bodydist x<=30 && (movecontact=[1,4]) && random<250

[State -1, CMP]
type=changestate
value=410
triggerall= var(59)>=1 && numenemy
triggerall= roundstate=2 && statetype=S && !inguarddist
triggerall= (p2bodydist x=[0,45]) && (p2dist y=[-50,50]) && p2statetype=C
triggerall= (p2stateno!=[120,155]) && p2movetype!=A && !(enemynear,hitfall)
trigger1= (stateno=200 || stateno=210 || stateno=230 || stateno=250)
trigger1= p2bodydist x<=20 && (movehit=[1,4]) && random<250

[State -1, CMK]
type=changestate
value=420
triggerall= var(59)>=1 && numenemy
triggerall= roundstate=2 && statetype=S && !inguarddist
triggerall= (p2bodydist x=[0,70]) && (p2dist y=[-50,50]) && p2statetype=S
triggerall= ((p2stateno!=[120,155]) || p2stateno=130 || p2stateno=150 || p2stateno=151) && p2movetype!=A
trigger1= (stateno=200 || stateno=210 || stateno=220 || stateno=230 || stateno=240 || stateno=250 || stateno=400 || stateno=430)
trigger1= p2bodydist x<=20 && (movehit=[1,4]) && random<250

[State -1, CLP]
type=changestate
value=400
triggerall= var(59)>=1 && numenemy
triggerall= roundstate=2 && statetype=S && !inguarddist && !(enemynear,hitfall)
triggerall= (p2bodydist x=[0,50]) && (p2dist y=[-50,50]) && p2statetype=C
triggerall= (p2stateno!=[120,155]) && p2movetype!=A
trigger1= ctrl && random<100
trigger2= (stateno=200 || stateno=230 || stateno=250) && time>=5 && random<50

[State -1, CLK]
type=changestate
value=430
triggerall= var(59)>=1 && numenemy
triggerall= roundstate=2 && statetype=S && !inguarddist
triggerall= (p2bodydist x=[0,36]) && (p2dist y=[-50,50]) && p2statetype=S
triggerall= ((p2stateno!=[120,155]) || p2stateno=130 || p2stateno=150 || p2stateno=151) && p2movetype!=A
trigger1= ctrl
trigger1= random<100 || (p2stateno=130 || p2stateno=150 || p2stateno=151) || p2stateno=5110
trigger2= (stateno=200 || stateno=230 || stateno=240) && time>=5 && random<50

[State -1, AHP]
type=changestate
value=420
triggerall= var(59)>=1 && numenemy
triggerall= roundstate=2 && statetype=S && !inguarddist
triggerall= (p2bodydist x=[0,45]) && (p2dist y=[-50,50]) && p2statetype!=L && !(enemynear,hitfall)
trigger1= ctrl && random<25
trigger2= (stateno=600 || stateno=630 || stateno=640 || stateno=610) && var(9)!=2 && (movehit=[1,4]) && random<250

[State -1, AHK]
type=changestate
value=450
triggerall= var(59)>=1 && numenemy
triggerall= roundstate=2 && statetype=S && !inguarddist
triggerall= (p2bodydist x=[0,50]) && (p2dist y=[-50,50]) && p2statetype!=L && !(enemynear,hitfall)
trigger1= ctrl && random<25
trigger2= (stateno=600 || stateno=630 || stateno=640 || stateno=610) && var(9)!=2 && (movehit=[1,4]) && random<250

[State -1, TKK]
type=changestate
value=1300
triggerall= var(59)>=1 && numenemy
triggerall= roundstate=2 && statetype=S
triggerall= vel x>0 && (vel y=[-3,3])
triggerall= (p2bodydist x=[0,45]) && (p2dist y=[-50,50])
trigger1= ctrl && random<25
trigger2= stateno=210 && animelemtime(8)>=5 && random<50

[State -1, AMP]
type=changestate
value=610
triggerall= var(59)>=1 && numenemy
triggerall= roundstate=2 && statetype=A
triggerall= (p2bodydist x=[0,100]) && (p2dist y=[-50,50]) && p2statetype!=L && !(enemynear,hitfall)
trigger1= ctrl && random<25

[State -1, AMK]
type=changestate
value=640
triggerall= var(59)>=1 && numenemy
triggerall= roundstate=2 && statetype=A
triggerall= (p2bodydist x=[-50,30]) && (p2dist y=[-50,50]) && p2statetype!=L && !(enemynear,hitfall)
trigger1= ctrl && random<25

[State -1, ALP]
type=changestate
value=600
triggerall= var(59)>=1 && numenemy
triggerall= roundstate=2 && statetype=A
triggerall= (p2bodydist x=[0,30]) && (p2dist y=[-50,50]) && p2statetype!=L && !(enemynear,hitfall)
trigger1= ctrl && random<25

[State -1, ALK]
type=changestate
value=630
triggerall= var(59)>=1 && numenemy
triggerall= roundstate=2 && statetype=A
triggerall= (p2bodydist x=[0,30]) && (p2dist y=[-50,50]) && p2statetype!=L && !(enemynear,hitfall)
trigger1= ctrl && random<25

[State -1, tatsumakizankuukyaku]
type=changestate
value=1200
triggerall= var(59)>=1 && numenemy
triggerall= roundstate=2 && statetype=S
triggerall= !(enemynear,ctrl) && (enemynear,stateno!=[120,155])
triggerall= (p2bodydist x=[0,90]) && (p2dist y=[-90,0]) && p2statetype!=L
triggerall= (enemynear,const(size.head.pos.y)<=-40) || (enemynear,statetype=A)
trigger1= (stateno=210 || stateno=225 || stateno=240)
trigger1= movehit && (p2bodydist x=[0,30]) && random<250
trigger2= stateno=250
trigger2= movehit && (p2bodydist x=[0,60]) && random<250

[State -1, hyakkishuu]
type=changestate
value=1100
triggerall= var(59)>=1 && numenemy
triggerall= roundstate=2 && statetype!=A
triggerall= (p2dist y=[-160,-80]) && p2statetype!=L
triggerall= !(enemynear,ctrl) && p2movetype=H && (enemynear,stateno!=[120,155])
trigger1= ctrl && random<10

[State -1, gouhadouken]
type=changestate
value=1200
triggerall= var(59)>=1 && numenemy
triggerall= roundstate=2 && statetype!=A
triggerall= ifelse(!var(20), (!numhelper(1000) && !numhelper(1025) && !numhelper(1055)),1) && !numhelper(3005) && !numhelper(3055)
triggerall= (p2bodydist x>=0) && (p2dist y>=-25) && p2movetype!=A && (p2statetype!=L || p2stateno=5120)
triggerall= (enemynear,const(size.head.pos.y)<=-40) || (enemynear,statetype=A)
trigger1= ctrl && p2bodydist x>=100 && random<100
trigger2= (stateno=[200,250])
trigger2= movehit && (p2bodydist x=[40,80]) && random<100

[State -1, shakunetsuhadouken]
type=changestate
value=1010
triggerall= var(59)>=1 && numenemy
triggerall= roundstate=2 && statetype!=A
triggerall= ifelse(!var(20), (!numhelper(1000) && !numhelper(1025) && !numhelper(1055)),1) && !numhelper(3005) && !numhelper(3055)
triggerall= (p2bodydist x>=0) && (p2dist y>=-25) && p2movetype!=A && p2statetype!=L
triggerall= (enemynear,const(size.head.pos.y)<=-40) || (enemynear,statetype=A)
trigger1= ctrl && p2bodydist x>=120 && random<50
trigger2= (stateno=210 || stateno=220 || stateno=240 || stateno=250)
trigger2= movehit && (p2bodydist x=[0,25]) && random<50

[State -1, shungokusatsu]
type=changestate
value=3000
triggerall= var(59)>=1 && numenemy
triggerall= roundstate=2 && statetype!=A
triggerall= power>=1000 && !var(20)
triggerall= !(enemynear,ctrl) && (p2stateno!=40) && (p2stateno!=[5030,5119])
triggerall= (p2bodydist x=[-160,160]) && (p2dist y=[-120,0]) && p2statetype!=L
triggerall= (enemynear,vel y=0) || (enemynear,vel y>0 && enemynear,vel x<0)
trigger1= ctrl && (p2bodydist x=[0,60]) && (enemynear,statetype!=A) && random<150
trigger2= stateno=210 && animelemtime(4)>=0 && (p2bodydist x=[0,50]) && p2statetype!=A && random<250

[State -1, misogi]
type=changestate
value=3000
triggerall= var(59)>=1 && numenemy
triggerall= roundstate=2 && statetype!=A
triggerall= power>=3000 && !var(20)
triggerall= !(enemynear,ctrl) && (enemynear,stateno!=[120,155]) && p2statetype!=L
triggerall= (p2stateno!=[5030,5119]) && (enemynear,vel x=[-1,1]) && (enemynear,vel y<4)
triggerall= movetype=A || !(enemynear,hitfall)
trigger1= ctrl && (enemynear,statetype=S || enemynear,statetype=C) && (enemynear,animtime<=-30) && random<100
trigger2= ctrl && (enemynear,statetype=A) && (enemynear,pos y<=-60) && (enemynear,movetype=A) && random<500
trigger3= stateno=1000 && movehit && random<100

[State -1, kkz]
type=changestate
value=3000
triggerall= var(59)>=1 && numenemy
triggerall= roundstate=2 && statetype!=A && prevstateno!=4200
triggerall= !numhelper(4205)
triggerall= power>=3000 && !var(20)
triggerall= !(enemynear,ctrl) && (enemynear,stateno!=[120,155])
triggerall= (p2bodydist x=[-30,30]) && (p2dist y=[-90,0])
trigger1= (stateno=1100 || stateno=3300) && movehit && random<50
trigger2= stateno=3000 && numhelper(3005)
trigger2= helper(3005),var(3) && random<100
trigger3= ctrl && inguarddist
trigger3=(p2stateno=[3000,4999]) && random<200

[State -1, tkj]
type=changestate
value=3000
triggerall= var(59)>=1 && numenemy
triggerall= roundstate=2 && statetype!=A && prevstateno!=4300
triggerall= !numhelper(4305)
triggerall= power>=3000 && !var(20)
triggerall= !(enemynear,ctrl) && (enemynear,stateno!=[120,155])
triggerall= (p2bodydist x=[-80,80]) && (p2dist y=[-60,0])
trigger1= stateno=210 && movehit && random<50
trigger2= stateno=240 && movehit && animelemtime(17)>=0 && random<200

[State -1, tenmagouzankuu2]
type=changestate
value=3000
triggerall= var(59)>=1 && numenemy
triggerall= roundstate=2 && statetype!=A && var(9)!=2 && prevstateno!=3070
triggerall= !numhelper(3075)
triggerall= power>=3000 && !var(20)
triggerall= !(enemynear,ctrl) && (enemynear,stateno!=[120,155])
triggerall= (p2bodydist x=[0,50]) && p2dist y>=-20
trigger1= (stateno=1100 || stateno=1250 || stateno=[1301,1303])
trigger1= movehit && (p2bodydist x=[0,35]) && random<250
trigger2= stateno=3100 && movehit && random<200
trigger3= (stateno=[3200,3250]) && movehit && (hitcount>=7 || anim=3205) && random<200
trigger4= stateno=3050 && numhelper(3055)
trigger4= helper(3055),var(3) && random<100

[State -1, messatsugoushoryuu]
type=changestate
value=3000
triggerall= var(59)>=1 && numenemy
triggerall= roundstate=2 && statetype!=A && prevstateno!=3100
triggerall= power>=3000 && var(20)<=60
triggerall= !(enemynear,ctrl) && (enemynear,stateno!=[120,155])
triggerall= (p2bodydist x=[-80,80]) && (p2dist y=[-40,0]) && p2statetype!=L
triggerall= (enemynear,const(size.head.pos.y)<=-40) || (enemynear,statetype=A)
trigger1= (stateno=210 || stateno=225 || stateno=240 || stateno=255)
trigger1= movehit && (p2bodydist x=[0,30]) && random<100
trigger2= (stateno=1100 || stateno=1305)
trigger2= movehit && (p2bodydist x=[0,30]) && random<250
trigger3= (stateno=[1000,4999]) && numhelper(stateno+5) && var(10)<=6
trigger3= helper(stateno+5),var(3) && random<100
trigger4= ctrl && enemynear,movetype=A && (p2bodydist x=[0,70]) && random<250

[State -1, messatsugousenpu]
type=changestate
value=3000
triggerall= var(59)>=1 && numenemy
triggerall= roundstate=2 && statetype!=A && var(9)!=2 && (prevstateno!=[3200,3250])
triggerall= power>=3000 && var(20)<=60
triggerall= !(enemynear,ctrl) && (enemynear,stateno!=[120,155])
triggerall= (p2bodydist x=[0,40]) && (p2dist y=[-90,0]) && p2statetype!=L
triggerall= (enemynear,const(size.head.pos.y)<=-40) || (enemynear,statetype=A)
trigger1= ((stateno=[1200,1250]) || stateno=[1301,1303])
trigger1= movehit && (p2bodydist x=[0,30]) && random<100
trigger2= stateno=3100 && movehit && random<333
trigger3= (stateno=[1000,4999]) && numhelper(stateno+5) && var(10)<=6 && stateno!=3070
trigger3= helper(stateno+5),var(3) && random<100

[State -1, messatsugourasen]
type=changestate
value=3000
triggerall= var(59)>=1 && numenemy
triggerall= roundstate=2 && statetype!=A && (prevstateno!=[3200,3250])
triggerall= power>=3000 && var(20)<=60
triggerall= !(enemynear,ctrl) && (enemynear,stateno!=[120,155])
triggerall= (p2bodydist x=[-45,45]) && (p2dist y=[-60,0]) && p2statetype!=L
trigger1= (stateno=210 || stateno=225 || stateno=240 || stateno=255)
trigger1= movehit && p2bodydist x=0 && random<100
trigger2= stateno=3100 && movehit && animelemtime(17)>=0 && p2bodydist x=0 && random<333
trigger3= stateno=4200 && numhelper(4205)
trigger3= helper(4205),var(3) && random<100
trigger4= ctrl && enemynear,movetype=A && (p2bodydist x=[0,10]) && random<200

[State -1, tenmashinzuiwari]
type=changestate
value=3000
triggerall= var(59)>=1 && numenemy
triggerall= roundstate=2 && statetype!=A && var(9)!=2 && pos y>=-50 && prevstateno!=3300
triggerall= power>=3000 && var(20)<=60
triggerall= !(enemynear,ctrl) && (enemynear,stateno!=[120,155])
triggerall= (p2bodydist x=[-30,60]) && p2dist y>=-15 && (enemynear,vel y<6)
trigger1= (stateno=270 || stateno=285)
trigger1= movehit && random<100
trigger2= (stateno=1100 || stateno=1250 || stateno=[1301,1303])
trigger2= movehit && random<250
trigger3= stateno=3100 && movehit && random<200
trigger4= (stateno=[3200,3250]) && movehit && (hitcount>=7 || anim=3205) && random<200
trigger5= stateno=1230 || stateno=3050 || stateno=3070
trigger5= ifelse(stateno=3070, animelemtime(31)>=0, 1)
trigger5= numhelper(stateno+5)
trigger5= helper(stateno+5),var(3) && random<200

[State -1, tenmagouzankuu]
type=changestate
value=3000
triggerall= var(59)>=1 && numenemy
triggerall= roundstate=2 && statetype!=A && var(9)!=2 && prevstateno!=3050
triggerall= !numhelper(3005) && !numhelper(3055)
triggerall= power>=1000 && var(20)<=60
triggerall= !(enemynear,ctrl) && (enemynear,stateno!=[120,155])
triggerall= (p2bodydist x=[0,120]) && p2dist y>=-20 && (enemynear,vel y<4)
trigger1= (stateno=270 || stateno=285)
trigger1= movehit && (p2bodydist x=[0,30]) && random<100
trigger2= (stateno=1100 || (stateno=[1200,1250]) || stateno=[1301,1303])
trigger2= movehit && random<250
trigger3= stateno=3100 && movehit && random<200
trigger4= (stateno=[3200,3250]) && movehit && (hitcount>=7 || anim=3205) && random<200
trigger5= (stateno=[1000,4999]) && numhelper(stateno+5) && var(10)<=6 && stateno!=3050
trigger5= helper(stateno+5),var(3) && random<100
trigger5= ifelse(stateno=3070, animelemtime(21)>=0, 1)

[State -1, messatsugouhadou]
type=changestate
value=3000
triggerall= var(59)>=1 && numenemy
triggerall= roundstate=2 && statetype!=A && prevstateno!=3000
triggerall= !numhelper(3005) && !numhelper(3055)
triggerall= power>=3000 && var(20)<=60
triggerall= !(enemynear,ctrl) && (enemynear,stateno!=[120,155])
triggerall= (p2bodydist x=[-120,120]) && (p2dist y=[-60,0]) && (enemynear,vel y<8) && p2statetype!=L
triggerall= (enemynear,const(size.head.pos.y)<=-40) || (enemynear,statetype=A)
trigger1= (stateno=1100 || stateno=1305)
trigger1= movehit && (p2bodydist x=[0,60]) && random<250
trigger2= stateno=3100 && movehit && animelemtime(17)>=0 && random<200
trigger3= (stateno=[1000,4999]) && numhelper(stateno+5) && var(10)<=6 && stateno!=3000
trigger3= helper(stateno+5),var(3) && random<100

[State -1, taunt]
type=changestate
value=195
triggerall= var(59)>=1 && numenemy
triggerall= !var(37)
triggerall= roundstate=2 && statetype!=A && prevstateno!=195
triggerall= life>=(lifemax/2)
trigger1= ctrl && numenemy
trigger1= (enemynear,life)<=(enemynear,lifemax/2)
trigger1= p2dist x>=160 && !(enemynear,ctrl) && (enemynear,movetype=H)
trigger1= (enemynear,statetype=A || enemynear,statetype=L) && random<500
