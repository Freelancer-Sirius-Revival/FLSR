# Mission Scripting

For each mission you may create one new `.ini` file with any name you wish in `Freelancer\EXE\flhook_plugins\missions\`.

This mission scripting system is mostly a re-implementation of Freelancer’s own mission scripting system. Although there are some differences due to changed requirements and use-cases in multiplayer.

You may start as many different mission at any time. However, only one instance of the same mission can run at the same time.

## Console Commands

- `start_mission <mission nickname>` Admin command (requires EVENT permission) to start this mission.
- `stop_mission <mission nickname>` Admin command (requires EVENT permission) to stop this mission.
- `reload_missions` Admin command (requires EVENT permission) to stop all missions, reload them again from files, and start initially active missions. Do not use too fast or server will crash due to system-blocked INI files.
- `act_trigger <mission nickname> <trigger nickname>` Admin command (requires EVENT permission) to activate the mission trigger. 
- `deact_trigger <mission nickname> <trigger nickname>` Admin command (requires EVENT permission) to deactivate the mission trigger.
- `getpos` Admin command (requires SUPERADMIN permission) to get the current ship position printed to chat.

## Syntax

The following explains the available sections and their key-values. Key-values are always written as
- `key`
    1. `STRING` The first value which is required.
    1. `[FLOAT]:42` Optional value that defaults to 42 and required floating point numbers.
    1. `ACTIVE|INACTIVE` This value has two allowed values: Active and Inactive.
    1. `[ACTIVE|INACTIVE] :INACTIVE` Optional value that allows Active or Inactive as value, and defaults to Inactive if not given.
- `[another key]` This key is optional.

## `Mission`

The head of any mission. All sections following this one will be assigned to this mission. This also means you can put multiple `Mission` into one file with their respective following contents.

To offer a mission on a Mission Board on bases, set at least `offer_type` and one base in `offer_bases`. The `initstate` will be ignored for those missions. A mission from the Mission Board will be only started once the mission was accepted. The player and group members accepting the mission will be automatically added to the label `players` for further reference in the mission.

- `nickname` The name of the mission. Used for admin commands and debug output.
    1. `STRING` The name. Must be unique among all missions on the server.

- `[initstate]` Whether this mission starts directly at server startup. Ignored if `offer_type` is given.
    1. `ACTIVE|INACTIVE :INACTIVE` The initial state.

- `[offer_type]`
    1. `DestroyShips|DestroyInstallation|DestroyContraband|Assassinate|CapturePrisoner|RetrieveContraband` Defining the icon displayed on the Mission Board.

- `[offer_target_system]`
    1. `STRING :0` The system nickname this mission will take the player to.

- `[offer_faction]`
    1. `STRING` The faction name offering this mission. If the player has less than the (default) -0.2 reputation with it, this mission will not be displayed.

- `[offer_string_id]`
    1. `INTEGER :0` The string id for the detailed Mission Board text box.

- `[offer_reward]`
    1. `INTEGER :0` Amount of cash displayed on the Mission Board as reward.

- `[offer_bases]`
    1. `STRING` A list of base nicknames this mission will be visible on the Mission Board.

## `[MsnSolar]`

This is the definition for a single solar for the mission. Multiple `MsnSolar` can be created for individual solars.

- `nickname` The object name of the solar. Used by trigger conditions and actions. Solar-Spawn-Plugin can use these by prefixing the name with `mission-nickname:`, e.g. `my_first_mission:some_station`.
    1. `STRING` The name. Must be unique in this mission.

- `[string_id]` The in-game display-name.
    1. `INTEGER` The resource ID to use.

- `system` The system to spawn into.
    1. `STRING` The system nickname.

- `position` Defines the position in space.
    1. `FLOAT :0` The x-axis.
    1. `FLOAT :0` The y-axis.
    1. `FLOAT :0` The z-axis.

- `[rotate]` Defines the rotation in space.
    1. `FLOAT :0` The x-axis.
    1. `FLOAT :0` The y-axis.
    1. `FLOAT :0` The z-axis.

- `archetype` Defines the archetype.
    1. `STRING` The solar archetype nickname.

- `[loadout]` Defines the equipment loadout.
    1. `STRING` The loadout nickname.

- `[hitpoints]` The absolute hitpoints to spawn this solar with.
    1. `[INTEGER|-1] :-1` The hitpoints. `-1` for full hitpoints, regardless the amount.

- `[base]` Only required for stations.
    1. `STRING` The base nickname the docks of this object will be connected with.

- `[faction]` If left empty, this object is not affiliated with anyone.
    1. `STRING` The faction nickname used.

- `[pilot]` The AI for guns.
    1. `STRING` The pilot nickname used.

- `[voice]` Required to allow comms sent from this base.
    1. `STRING` The voice nickname used.

- `[space_costume]` Required to make a comms window visible to the player on interaction.
    1. `[STRING]` The head nickname used.
    1. `[STRING]` The body nickname used.
    1. `[STRING]` The 1. accessory slot used.
    1. `[STRING]` The 2. accessory slot used.
    1. `[STRING]` The 3. accessory slot used.
    1. `[STRING]` The 4. accessory slot used.
    1. `[STRING]` The 5. accessory slot used.
    1. `[STRING]` The 6. accessory slot used.
    1. `[STRING]` The 7. accessory slot used.
    1. `[STRING]` The 8. accessory slot used.

- `[label]` Can be defined multiple times. Places this object into a group with other likewise labeled objects.
    1. `STRING` Name of the group to be linked with.

## `[Npc]`

This is the definition for a NPC archetype which again can be used by multiple `MsnNpc`. Multiple entries of this can exist.

- `nickname` The NPC name refered by `MsnNpc`.
    1. `STRING` The name. Must be unique in this mission.

- `archetype` Defines the archetype.
    1. `STRING` The solar archetype nickname.

- `[loadout]` Defines the equipment loadout.
    1. `STRING` The loadout nickname.

- `[state_graph]` Basic behaviour of the ship.
    1. `STRING` The state graph to use.

- `[faction]` If left empty, this object is not affiliated with anyone.
    1. `STRING` The faction nickname used.

- `[pilot]` The AI for the pilot.
    1. `STRING` The pilot nickname used.

- `[voice]` Required to allow comms sent from this base.
    1. `STRING` The voice nickname used.

- `[space_costume]` Required to make a comms window visible to the player on interaction.
    1. `[STRING]` The head nickname used.
    1. `[STRING]` The body nickname used.
    1. `[STRING]` The 1. accessory slot used.
    1. `[STRING]` The 2. accessory slot used.
    1. `[STRING]` The 3. accessory slot used.
    1. `[STRING]` The 4. accessory slot used.
    1. `[STRING]` The 5. accessory slot used.
    1. `[STRING]` The 6. accessory slot used.
    1. `[STRING]` The 7. accessory slot used.
    1. `[STRING]` The 8. accessory slot used.

- `[level]` Sets the displayed level.
    1. `INTEGER` The level to show. Can be from 0 to 255.

## `[MsnNpc]`

This is the definition for a single NPC for the mission. Multiple `MsnNpc` can be created for individual NPCs.

- `nickname` The object name of the NPC. Used by trigger conditions and actions.
    1. `STRING` The name. Must be unique in this mission.

- `[string_id]` The in-game display-name. If not given, a random name based on the used faction will be generated.
    1. `INTEGER` The resource ID to use.

- `system` The system to spawn into.
    1. `STRING` The system nickname.

- `position` Defines the position in space.
    1. `FLOAT :0` The x-axis.
    1. `FLOAT :0` The y-axis.
    1. `FLOAT :0` The z-axis.

- `[rotate]` Defines the rotation in space.
    1. `FLOAT :0` The x-axis.
    1. `FLOAT :0` The y-axis.
    1. `FLOAT :0` The z-axis.

- `npc` Defines the NPC archetype.
    1. `STRING` The `NPC` archetype nickname defined in this mission. See previous section.

- `[hitpoints]` The absolute hitpoints to spawn this NPC with.
    1. `[INTEGER|-1] :-1` The hitpoints. `-1` for full hitpoints, regardless the amount.

- `[pilot_job]` Override for the job of the pilot.
    1. `STRING` The job nickname used.

- `[arrival_obj]` Launches from a base or jump-object.
    1. `STRING` The solar nickname to undock at spawn from.

- `[label]` Can be defined multiple times. Places this object into a group with other likewise labeled objects.
    1. `STRING` Name of the group to be linked with.

## `[MsnFormation]`

This is the definition for an NPC formation. Multiple `MsnFormation` can be created for individual NPCs.

- `nickname` The name of the formation. Used by trigger actions.
    1. `STRING` The name. Must be unique in this mission.

- `formation` The formation from `DATA/MISSIONS/formations.ini`.
    1. `STRING` The formation name.

- `ship` Multiple entries allowed. The ship to spawn. The order of these entries defines their position in the formation.
    1. `STRING` The `MsnShip` nickname.

- `position` Defines the position in space.
    1. `FLOAT :0` The x-axis.
    1. `FLOAT :0` The y-axis.
    1. `FLOAT :0` The z-axis.

- `[rotate]` Defines the rotation in space.
    1. `FLOAT :0` The x-axis.
    1. `FLOAT :0` The y-axis.
    1. `FLOAT :0` The z-axis.

## `[ObjList]`

Objectives define a list of directives for NPCs to follow along. They can be assigned to NPCs as often as necessary.

- `nickname` The objective list name. Used by actions.
    1. `STRING` The name. Must be unique in this mission.

### Objectives

- `Delay` Pauses for a time.
    1. `INTEGER: :0` The time to pause.

- `Dock` Fly toward a specific object and dock to it.
    1. `STRING` Object by name to dock to.

- `Follow` Follows the given target.
    1. `STRING` The target name ship to follow.
    1. `FLOAT :0` Maximum distance not to exceed.
    1. `FLOAT :0` The relative x-axis position to the target.
    1. `FLOAT :0` The relative y-axis position to the target.
    1. `FLOAT :0` The relative z-axis position to the target.

- `GotoObj` Fly toward a specific object in space.
    1. `Goto_Cruise|Goto_No_Cruise` Whether to fly in cruise or not.
    1. `STRING` Object by name to fly to.
    1. `FLOAT :0` The distance from the given position to stop at.
    1. `FLOAT|-1 :0` The absolute thrust speed to fly at. `-1` for max speed.
    1. `[STRING]` Object by name to wait for.
    1. `[FLOAT] :0` Distance to begin slowing down to wait for object.
    1. `[FLOAT] :0` Distance to come to a full stop to wait for object.

- `GotoSpline` Fly toward a specific point in space along a spline.
    1. `Goto_Cruise|Goto_No_Cruise` Whether to fly in cruise or not.
    1. `FLOAT :0` The x-axis position of point 1.
    1. `FLOAT :0` The y-axis position of point 1.
    1. `FLOAT :0` The z-axis position of point 1.
    1. `FLOAT :0` The x-axis position of point 2.
    1. `FLOAT :0` The y-axis position of point 2.
    1. `FLOAT :0` The z-axis position of point 2.
    1. `FLOAT :0` The x-axis position of point 3.
    1. `FLOAT :0` The y-axis position of point 3.
    1. `FLOAT :0` The z-axis position of point 3.
    1. `FLOAT :0` The x-axis position of point 4.
    1. `FLOAT :0` The y-axis position of point 4.
    1. `FLOAT :0` The z-axis position of point 4.
    1. `FLOAT :0` The distance from the given position to stop at.
    1. `FLOAT|-1 :0` The absolute thrust speed to fly at. `-1` for max speed.
    1. `[STRING]` Object by name to wait for.
    1. `[FLOAT] :0` Distance to begin slowing down to wait for object.
    1. `[FLOAT] :0` Distance to come to a full stop to wait for object.

- `GotoVec` Fly toward a specific point in space.
    1. `Goto_Cruise|Goto_No_Cruise` Whether to fly in cruise or not.
    1. `FLOAT :0` The x-axis position for the target.
    1. `FLOAT :0` The y-axis position for the target.
    1. `FLOAT :0` The z-axis position for the target.
    1. `FLOAT :0` The distance from the given position to stop at.
    1. `FLOAT|-1 :0` The absolute thrust speed to fly at. `-1` for max speed.
    1. `[STRING]` Object by name to wait for.
    1. `[FLOAT] :0` Distance to begin slowing down to wait for object.
    1. `[FLOAT] :0` Distance to come to a full stop to wait for object.

## `[Trigger]`

Triggers are the core logical elements of a mission. Multiple `Trigger` can be created for a mission. They always must contain a singular condition (`Cnd_`) which must be fulfilled to execute all actions (`Act_`). A condition of a trigger only can be fulfilled if the trigger is activated. Triggers are usually deactivated by default and should be activated as the mission progresses.

- `nickname` The name of the trigger. Used as reference for following actions and for debug output.
    1. `STRING` The name. Must be unique within this mission.
    
- `[initstate]` Whether this trigger gets activated directly on mission start.
    1. `Active|Inactive :Inactive` The initial state.

- `[repeatable]` Allows the trigger to be executed more than once.
    1. `Off|Auto|Manual :Off` `Auto` instantly re-activates the trigger upon finished execution until being deactivated via `Act_DeactTrig`. `Manual` deactivates the trigger after finished execution until being re-activated via `Act_ActTrig`.

### Conditions

Only one condition must be present. If none is present, `Cnd_True` is used.

The keyword `Stranger` is used to refer explicitely to all players not having a label assigned in this mission. It can be used in combination with action’s `Activator` to assign players to the mission.

- `Cnd_BaseEnter` Only for players. Landing or logging in to bases.
    1. `STRING|Stranger` The players to await fully landing on a base.
    1. `[STRING] :any` The base nickname the player lands on.

- `Cnd_Cloaked` Checks whether the object is cloaked or not (>90% invisibility progress)
    1. `STRING|Stranger` Object by name or label to watch cloaking state of.
    1. `True|False :False` `False` if the target must be uncloaked, or `True` if it must be cloaked.

- `Cnd_CommComplete` Waits for a comm to complete. See `Act_SendComm` and `Act_EtherComm`.
    1. `STRING` The comm name to wait finishing for. Started comms not ending within 10 seconds will trigger `Cnd_CommComplete` to allow continuing the flow even if no player witnessed hearing it.

- `Cnd_Count` Counts the objects from a label. `Activator` will be the server.
    1. `STRING` Objects by label to count.
    1. `INTEGER` The target count.
    1. `[Less|Equal|Greater] :Equal` The comparator against the target count.

- `Cnd_Destroyed` When something gets destroyed/despawned. `Activator` can be the server when objects get despawned.
    1. `STRING|Stranger` Object by name or label to await the destruction of.
    1. `[INTEGER] :0` The specific count of objects that must be destroyed to meet this condition. Any negative value will make this wait until all occurances of the objects are destroyed.
    1. `[Explode|Vanish|Any] :Any` Expects the destruction to be either by violence (`Explode`), despawn (e.g. by docking or explicit despawn) (`Vanish`) or whatever reason (`Any`).
    1. `[STRING]` Object by name or label that must be the killer.

- `Cnd_DistObj` Distance to another object in space.
    1. `STRING|Stranger` Must be a ship. Object by name or label to expect within the distance.
    1. `STRING` Other object by name or label to expect within the distance. Can be a static world solar.
    1. `FLOAT :0` The distance from the given position to check.
    1. `[Inside|Outside] :Inside` Whether the objects must be within or outside this distance.

- `Cnd_DistVec` Distance from a vector in space.
    1. `STRING|Stranger` Must be a ship. Object by name or label to expect within the distance.
    1. `FLOAT :0` The x-axis position for this volume.
    1. `FLOAT :0` The y-axis position for this volume.
    1. `FLOAT :0` The z-axis position for this volume.
    1. `FLOAT :0` The distance from the given position to check.
    1. `STRING` The system nickname to place this volume into.
    1. `[Inside|Outside] :Inside` Whether the objects must be within or outside this distance.

- `Cnd_HealthDec` When the hitpoints falls below a threshold.
    1. `STRING|Stranger` Object by name or label to observe the hitpoints of.
    1. `FLOAT :0` The percentage of hitpoints the target must lose before the damage prevention kicks in.
    1. `[Root|STRING] :Root` Multiple subsequent entries possible. Lists the target’s collision groups to check the hitpoint loss of.

- `Cnd_ProjHitCount` Counts projectile hits to the target.
    1. `STRING` Object by name or label to count projectile hits on.
    1. `[INTEGER] :1` The count of projectile hits that must have happened.
    1. `[Any|Hull|Shield] :Any` The surface the hit should be registered on.
    1. `[Any|Projectile|Explosion] :Any` The damage type to register. `Any` means `Projectile` and `Explosion`.
    1. `[STRING|Stranger]` Damage inflictor object by name or label. If none is given, defaults to all mission objects and players.

- `Cnd_SpaceEnter` Only for players. Undocking from bases or spawning into space. Not jumping.
    1. `STRING|Stranger` The players  to await spawning into space.
    1. `[STRING] :any` The system nickname the player spawns into.

- `Cnd_SpaceExit` Only for players. Docking to bases, destruction, leaving the server, or changing to another character. Not jumping.
    1. `STRING|Stranger` The players to await despawning from space.
    1. `[STRING] :any` The system nickname the player despawns from.

- `Cnd_Timer` Waits until the time as passed. `Activator` will be the server.
    1. `FLOAT` Lower limit of randomized time.
    1. `[FLOAT] :0` Upper limit of randomized time. When not set or lower than the lower limit, the timer will always use a constant time of lthe lower limit.

- `Cnd_True` No values. This instantly lets the trigger execute. `Activator` will be the server.

### Actions

There can be as many actions as needed – even the same ones.

The keyword `Activator` is used to refer explicitely to the object/player that fulfilled the trigger condition. Sometimes this can be the server itself (e.g. `Cnd_True`, `Cnd_Timer`). The `Activator` can be used in combination with the condition’s `Stranger` to assign players to the mission or do other things only to non-assigned players.

- `Act_DebugMsg` Prints a message into Hook console and to all players registered to the mission.
    1. `STRING` Arbitrary text to print.

- `Act_EndMission` No values. Ends the mission and cleans up all spawned objects, waypoints, and music. This must be called to allow the mission to be re-started. Admin command `stop_mission` does the same.

- `Act_ChangeState` Sets the mission state. This does not end the mission!
    1. `SUCCEED|FAIL :FAIL` The state to change into. Displays and plays respective text to all players of the mission. On `FAIL` it will re-offer the mission on the job board if applicable.
    1. `INTEGER :0` Only for `FAIL`: The text ID to display.

- `Act_ActTrig` Activates a trigger.
    1. `STRING` Trigger nickname to refer.
    1. `[FLOAT] :1` A probability between `0` and `1` the trigger will be activated or not.

- `Act_DeactTrig` Deactivates a trigger.
    1. `STRING` Trigger nickname to refer.
    1. `[FLOAT] :1` A probability between `0` and `1` the trigger will be deactivated or not.

- `Act_ActMsn` Activates another mission.
    1. `STRING` Mission nickname to refer.
    1. `[STRING|All]` Multiple subsequent entries possible. A label whose assigned players are transferred over to the mission with the same label. `All` transfers over all players and their labels.

- `Act_ActMsnTrig` Activates a trigger of a another mission.
    1. `STRING` Mission nickname to refer.
    1. `STRING` Trigger nickname to refer.
    1. `[FLOAT] :1` A probability between `0` and `1` the trigger will be activated or not.

- `Act_DeactMsnTrig` Deactivates a trigger of a another mission.
    1. `STRING` Mission nickname to refer.
    1. `STRING` Trigger nickname to refer.
    1. `[FLOAT] :1` A probability between `0` and `1` the trigger will be deactivated or not.

- `Act_AddLabel` Adds a label to the objects. **This is the only way to assign players to the mission.**
    1. `STRING|Activator` Object by name or label to manipulate.
    1. `STRING` The label to add.

- `Act_RemoveLabel` Removes a label to the objects. **This is the only explicit way to unassign players from the mission.**
    1. `STRING|Activator` Object by name or label to manipulate.
    1. `STRING` The label to remove.

- `Act_SpawnSolar` Spawns a solar. Only one instance of it can exist at the same time.
    1. `STRING` The `MsnSolar` nickname to spawn.

- `Act_SpawnShip` Spawns a ship. Only one instance of it can exist at the same time.
    1. `STRING` The `MsnNpc` nickname to spawn.
    1, `[STRING|no_ol] :no_ol` The initial `ObjList` to spawn with. `no_ol` for none.
    1. `[FLOAT] :0` Override for initial x-axis position.
    1. `[FLOAT] :0` Override for initial y-axis position.
    1. `[FLOAT] :0` Override for initial z-axis position.
    1. `[FLOAT] :0` Override for initial x-axis rotation.
    1. `[FLOAT] :0` Override for initial y-axis rotation.
    1. `[FLOAT] :0` Override for initial z-axis rotation.

- `Act_SpawnFormation` Spawns a formation of ships.
    1. `STRING` The `MsnFormation` nickname to spawn.
    1, `[STRING|no_ol] :no_ol` The initial `ObjList` to spawn with. `no_ol` for none.
    1. `[FLOAT] :0` Override for initial x-axis position.
    1. `[FLOAT] :0` Override for initial y-axis position.
    1. `[FLOAT] :0` Override for initial z-axis position.
    1. `[FLOAT] :0` Override for initial x-axis rotation.
    1. `[FLOAT] :0` Override for initial y-axis rotation.
    1. `[FLOAT] :0` Override for initial z-axis rotation.

- `Act_Destroy` Destroys an object.
    1. `STRING|Activator` Object by name or label to destroy.
    1. `[Explode|Silent] :Silent` Whether to explode the object or despawn it. Explosion does *not* trigger the death fuse.

- `Act_Relocate` Relocates an object.
    1. `STRING|Activator` Object by name to relocate.
    1. `FLOAT :0` Position on x-axis.
    1. `FLOAT :0` Position on y-axis.
    1. `FLOAT :0` Position on z-axis.
    1. `[FLOAT]` Override for current x-axis rotation.
    1. `[FLOAT]` Override for current y-axis rotation.
    1. `[FLOAT]` Override for current z-axis rotation.

- `Act_LightFuse` Executes an arbitary fuse.
    1. `STRING|Activator` Object by name or label to refer.
    1. `STRING` Fuse nickname to execute on the objects.
    1. `[FLOAT]` The time-offset between `0` and `1` to start the fuse from.
    1. `[FLOAT]` Overrides the fuse lifetime by this value.

- `Act_GiveObjList` Gives NPCs a list of objectives.
    1. `STRING|Activator` The NPCs to receive the objectives.
    1. `STRING` The `ObjList` nickname to refer to.

- `Act_SetNNObj` Only for players. Sets their current objective. For a waypoint the system and position must be given. It will clear all waypoints if the system is not specified.
    1. `STRING|Activator` The players to set the message or waypoint.
    1. `[INTEGER] :0` Resource ID to display as message to the players. `0` shows no message.
    1. `[STRING]` The system nickname for the waypoint.
    1. `[FLOAT] :0` The x-axis position for the waypoint.
    1. `[FLOAT] :0` The y-axis position for the waypoint.
    1. `[FLOAT] :0` The z-axis position for the waypoint.
    1. `[True|False] :False` Whether this should be not a singular waypoint but an actual best-path route. **Best route may not work if the player does not have relevant system connections discovered.**
    1. `[STRING]` The optional object nickname to specify as waypoint destination. Not limited to the mission; this can be any static world solar.

- `Act_PlaySoundEffect` Only for players. Plays a single sound effect. This is *not* audible for other players.
    1. `STRING|Activator` The players to play the sound effect for.
    1. `STRING` The sound nickname to play.

- `Act_PlayMusic` Only for players. Sets the music. This will remain until music is reset by all values being `None`, player changes system, player docks, logs out from character.
    1. `STRING|Activator` The players to set music for.
    1. `[STRING|None] :None` Overrides the space music.
    1. `[STRING|None] :None` Overrides the danger music.
    1. `[STRING|None] :None` Overrides the battle music.
    1. `[STRING|None] :None` Overrides all music by this track.
    1. `[FLOAT] :0` The time in seconds it takes to transition music.
    1. `[True|False] :False` Whether to play the override music (5) only once and then return to other music.

- `Act_SendComm` Sends communication from one object to others. A sender without proper space costume will not display a comms window. Note that players cannot receive such comms from objects that are not present at the client (e.g. NPCs outside their spawn/sync range).
    1. `STRING` The name of this comm. Referred to by `Cnd_CommComplete`.
    1. `STRING|Activator` Object by name or label to receive this comm.
    1. `STRING` Object by name to send this comm. Can be a static world solar. Must have a voice defined. Cannot be a player.
    1. `STRING` Multiple subsequent entries possible. Must be one of the defined voice’s sound messages. 
    1. `[FLOAT] : 0` The additional delay after this comm has ended before any other comm can reach the receiver. Also influences when the comm is considered complete.
    1. `[True|False] :False` Whether this comm can be heard by bystanders in space.

- `Act_Ethercomm` Sends communication from no specific source to others. A sender without proper space costume will not display a comms window.
    1. `STRING` The name of this comm. Referred to by `Cnd_CommComplete`.
    1. `STRING|Activator` Object by name or label to receive this comm.
    1. `STRING` The voice nickname to use for the sender.
    1. `STRING` Multiple subsequent entries possible. Must be one of the defined voice’s sound messages. 
    1. `[FLOAT] :0` The additional delay after this comm has ended before any other comm can reach the receiver. Also influences when the comm is considered complete.
    1. `[True|False] :False` Whether this comm can be heard by bystanders in space.
    1. `[INTEGER] :0` The sender’s resource ID to display as name below the comms window.
    1. `[STRING]` The head nickname used for the sender.
    1. `[STRING]` The body nickname used for the sender.
    1. `[STRING]` The 1. accessory slot used for the sender.
    1. `[STRING]` The 2. accessory slot used for the sender.
    1. `[STRING]` The 3. accessory slot used for the sender.
    1. `[STRING]` The 4. accessory slot used for the sender..
    1. `[STRING]` The 5. accessory slot used for the sender.
    1. `[STRING]` The 6. accessory slot used for the sender.
    1. `[STRING]` The 7. accessory slot used for the sender.
    1. `[STRING]` The 8. accessory slot used for the sender.

- `Act_AdjAcct` Only for players. Adjusts the cash on the account. Cash will be automatically clamped to prevent overflows/underflows.
    1. `STRING|Activator` The players to have their cash being modified.
    1. `INTEGER :0` A positive or negative number of cash. Cannot exceed more than +-(2^32)-1.
    1. `[True|False] :False` Split the cash across all player members of the receiving label equally. No effect if a singular player receives the cash.

- `Act_AdjRep` Only for players. Adjusts the reputation of the player toward a specific faction.
    1. `STRING|Activator` The players to have their reputation being modified.
    1. `STRING` The faction name to change reputation toward. Relative changes according to `empathy.ini` will be computed.
    1. `FLOAT|ObjectDestruction|MissionSuccess|MissionFailure|MissionAbortion :ObjectDestruction` The change magnitue. Either uses a given value, or takes one of the predefined events from `empathy.ini`.
    
- `Act_AddCargo` Only for players. Adds cargo. Will fail if player has not enough cargo space.
    1. `STRING|Activator` The players to receive the cargo.
    1. `STRING` The item nickname to use.
    1. `INTEGER :0` Amount of items to add to cargo.
    1. `[True|False :False]` Whether this cargo is flagged as mission item.

- `Act_SetVibe` Sets the vibe/attitude uni-directional between two targets. For label members that join later this action must be invoked again.
    1. `STRING|Activator` Object by name or label whose vibe will be set. For players this only works if the target is another player.
    1. `STRING|Activator` Object by name or label to change the vibe toward. For players it will automatically change their vibe in turn, too. If both arguments are players, it will stay uni-directional.
    1. `FLOAT :0` The reputation value between `-1` and `1` to set the vibe on.

- `Act_Invulnerable` Sets invulnerability for the target.
    1. `STRING|Activator` Object by name or label for which invulnerability will be set.
    1. `True|False :False` Whether to prevent non-player damage (NPCs, radiation, asteroid mines) or not.
    1. `[True|False] :False` Whether to prevent player damage or not.
    1. `[FLOAT] :0` The percentage of hitpoints the target must lose before the damage prevention kicks in.
