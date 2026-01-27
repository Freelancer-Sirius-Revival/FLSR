# Mission Scripting

For each mission you may create one new `.ini` file with any name you wish in `Freelancer\EXE\flhook_plugins\missions\`.

This mission scripting system is mostly a re-implementation of Freelancer’s own mission scripting system. Although there are some differences due to changed requirements and use-cases in multiplayer.

You may start as many different mission at any time. However, only one instance of the same mission can run at the same time.

## Console Commands

- `start_mission <mission nickname>` Admin command (requires EVENT permission) to start this mission.
- `stop_mission <mission nickname>` Admin command (requires EVENT permission) to stop this mission. Does the same as `Act_TerminateMsn`.
- `reload_missions` Admin command (requires EVENT permission) to stop all missions, reload them again from files, and start initially active missions. Do not use too fast or server will crash due to system-blocked INI files.
- `act_trigger <mission nickname> <trigger nickname>` Admin command (requires EVENT permission) to activate the mission trigger. 
- `deact_trigger <mission nickname> <trigger nickname>` Admin command (requires EVENT permission) to deactivate the mission trigger.
- `getpos` Admin command (requires SUPERADMIN permission) to get the current ship position printed to chat.

## Syntax

The following explains the available sections and their key-values. Key-values are always written as
- `key`
    1. `STRING` The first value which is required.
    1. `[FLOAT] :42` Optional value that defaults to 42 and required floating point numbers.
    1. `Active|Inactive` This value has two allowed values: Active and Inactive.
    1. `[Active|Inactive] :Inactive` Optional value that allows Active or Inactive as value, and defaults to Inactive if not given.
- `[another key]` This key is optional.

## `Mission`

The head of any mission. All sections following this one will be assigned to this mission. This also means you can put multiple `Mission` into one file with their respective following contents.

To offer a mission on a Mission Board on bases, set at least `offer_type` and one base in `offer_bases`. The `initstate` will be ignored for those missions. A mission from the Mission Board will be only started once the mission was accepted. The player The player and their group will be instantly given the label `players`. The player who accepted the mission will also be labelled `initial_player`.

- `nickname` The name of the mission. Used for admin commands and debug output.
    1. `STRING` The name. Must be unique among all missions on the server.

- `[initstate]` Whether this mission starts directly at server startup. Ignored if `offer_type` is given.
    1. `Active|Inactive :Inactive` The initial state.

- `[offer_type]`
    1. `DestroyShips|DestroyInstallation|DestroyContraband|Assassinate|CapturePrisoner|RetrieveContraband` Defining the icon displayed on the Mission Board.

- `[offer_target_system]`
    1. `STRING :0` The system nickname this mission will take the player to.

- `[offer_faction]`
    1. `STRING` The faction name offering this mission. If the player has less than the (default) -0.2 reputation with it, this mission will not be displayed.

- `[offer_title_id]`
    1. `INTEGER :0` The string id for the Mission title.
    
- `[offer_description_id]`
    1. `INTEGER :0` The string id for the detailed Mission Board text box.

- `[offer_reward]`
    1. `INTEGER :0` Amount of cash displayed on the Mission Board as reward.

- `[offer_ship_restriction]`
    1. `STRING` A list of ship nicknames that are allowed to take this mission. It will be displayed and declined with invalid ship.

- `[offer_bases]`
    1. `STRING` A list of base nicknames this mission will be visible on the Mission Board.

- `[reoffer]`
    1. `Always|OnFail|OnSuccess|Never :Never` The condition under which the mission will be reoffered. Set by `Act_SetMsnResult`.

- `[reoffer_delay]`
    1. `FLOAT :0` The time in seconds to wait before the mission is reoffered.

## `MsnSolar`

This is the definition for a single solar for the mission. Multiple `MsnSolar` can be created for individual solars.
The hitpoints defined in the archetype will always apply, even if not set as `destructible`. The `destructible` flag only decides whether the health bar is displayed to players. As precaution, any non `destructible` solars will automatically spawned invincible. This can be overriden by using `Act_Invulnerable` after spawn (e.g. to allow full destruction of equipment).

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

## `Npc`

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

- `[level]` Sets the displayed level.
    1. `INTEGER` The level to show. Can be from 0 to 255.

## `[MsnNpc]`

This is the definition for a single NPC for the mission. Multiple `MsnNpc` can be created for individual NPCs.

**NPCs are only simulated if at least one player is in space of the same system (dead or alive). Their simulation will be completely paused if the last player leaves the system space.**

- `nickname` The object name of the NPC. Used by trigger conditions and actions.
    1. `STRING` The name. Must be unique in this mission.

- `[string_id]` The in-game display-name. If not given, a random name based on the used faction will be generated. If no name can be generated, uses the ship name.
    1. `INTEGER` The resource ID to use.

- `[use_ship_ids]` Use the ship archetype IDS Name as in-game display-name.
    1. `True|False :False` Whether to use the ship IDS Name or the `string_id`.

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

- `[voice]` Required to allow comms sent from this base. Takes a random one by the NPC faction if empty.
    1. `STRING` The voice nickname used.

- `[space_costume]` Required to make a comms window visible to the player on interaction. Takes a random one by the NPC faction if empty.
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

- `[hitpoints]` The absolute hitpoints to spawn this NPC with.
    1. `[INTEGER|-1] :-1` The hitpoints. `-1` for full hitpoints, regardless the amount.

- `[pilot_job]` Override for the job of the pilot.
    1. `STRING` The job nickname used.

- `[arrival_obj]` Launches from a solar or NPC. They must each have valid `docking_sphere` entries.
    1. `STRING` The object nickname to spawn from.

- `[label]` Can be defined multiple times. Places this object into a group with other likewise labeled objects.
    1. `STRING` Name of the group to be linked with.

## `MsnFormation`

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

## `ObjList`

Objectives define a list of directives for NPCs to follow along. They can be assigned to NPCs as often as necessary.

**Objectives only continue to be executed if at least one player is in space of the same system (dead or alive) as the NPC. As long as there is no player, Objective progression will be paused.**

- `nickname` The objective list name. Used by actions.
    1. `STRING` The name. Must be unique in this mission.

### Objectives

- `BreakFormation` Breaks from the current formation. When formation leader, it dissolves the formation entirely.

- `Delay` Enforce a pause of any action or maneuver for a time.
    1. `INTEGER :0` The time to pause.

- `Dock` Fly toward a specific object and dock to it. When the NPC is formation leader, it will dissolve the formation and tell every member to dock, too.
    1. `STRING` Object by name to dock to. Can also be a static world solar.

- `Follow` Follows the given target.
    1. `STRING` The target name ship to follow.
    1. `FLOAT :0` The relative x-axis position to the target.
    1. `FLOAT :0` The relative y-axis position to the target.
    1. `FLOAT :0` The relative z-axis position to the target.
    1. `[FLOAT] :100` Maximum distance not to exceed.

- `GotoObj` Fly toward a specific object in space.
    1. `Goto_Cruise|Goto_No_Cruise` Whether to fly in cruise or not.
    1. `STRING` Object by name to fly to. Can also be a static world solar.
    1. `[FLOAT] :100` The distance from the given position to stop at.
    1. `[FLOAT|-1] :-1` The absolute thrust speed to fly at. `-1` for max speed.
    1. `[STRING]` Object by name to wait for.
    1. `[FLOAT] :200` Distance to begin slowing down to wait for object.
    1. `[FLOAT] :500` Distance to come to a full stop to wait for object.

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
    1. `[FLOAT] :100` The distance from the given position to stop at.
    1. `[FLOAT|-1] :-1` The absolute thrust speed to fly at. `-1` for max speed.
    1. `[STRING]` Object by name to wait for.
    1. `[FLOAT] :200` Distance to begin slowing down to wait for object.
    1. `[FLOAT] :500` Distance to come to a full stop to wait for object.

- `GotoVec` Fly toward a specific point in space.
    1. `Goto_Cruise|Goto_No_Cruise` Whether to fly in cruise or not.
    1. `FLOAT :0` The x-axis position for the target.
    1. `FLOAT :0` The y-axis position for the target.
    1. `FLOAT :0` The z-axis position for the target.
    1. `[FLOAT] :100` The distance from the given position to stop at.
    1. `[FLOAT|-1] :-1` The absolute thrust speed to fly at. `-1` for max speed.
    1. `[STRING]` Object by name to wait for.
    1. `[FLOAT] :200` Distance to begin slowing down to wait for object.
    1. `[FLOAT] :500` Distance to come to a full stop to wait for object.

- `Idle` Enforce idle mode, allowing automatic NPC behaviour to resume.

- `MakeNewFormation` Create a new formation together with other ships.
    1. `STRING` The formation from `DATA/MISSIONS/formations.ini`.
    1. `STRING` Multiple subsequent entries possible. The `MsnNpc` names of ships to add to this formation, in that order.

- `SetLifeTime` Once the NPC comes out of range from all players, its life time will be counted down. The life time will be restored completely if any player comes back into range to the NPC. All NPCs of a formation will be despawned if one of their members is running out of life time.
    1. `FLOAT :1` The life time in seconds. `-1` means infinite life time.

- `SetPriority` Sets the priority for all following objectives.
    1. `Normal|Always_Execute` Whether the objectives can be paused by fights, or are enforced to ignore any disturbances.

- `StayInRange` Enforces staying within range to the target until actively released. This comes with overloaded arguments as by vanilla Freelancer:

    - For a target object:
    1. `STRING` Object by name to stay close to. Can also be a static world solar.
    1. `[FLOAT] :100` The maximum range to stay from the object.
    1. `[True|False] :True` Whether `StayInRange` is enforced (`True`) or released (`False`).

    - For a target position:
    1. `FLOAT` The x-axis position for the target.
    1. `FLOAT` The y-axis position for the target.
    1. `FLOAT` The z-axis position for the target.
    1. `[FLOAT] :100` The maximum range to stay from the object.
    1. `[True|False] :True` Whether `StayInRange` is enforced (`True`) or released (`False`).

## `Dialog`

Dialogs are a shortcut to defining multiple `Act_SendComm` or `Act_EtherComm` for complex dialogs.

- `nickname` The dialog name. Used by `Act_StartDialog`.
    1. `STRING` The name. Must be unique in this mission.
- `etherSender` A sender definition coming from no specific object (see `Act_EtherComm`).
    1. `STRING` The name of this sender.
    1. `STRING` The voice nickname to use for the sender.
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
- `line` In definition order: Sends communication from one object to others. A sender without proper space costume will not display a comms window. Note that players cannot receive such comms from objects that are not present at the client (e.g. NPCs outside their spawn/sync range).
    1. `STRING` The name of this comm. Referred to by `Cnd_CommComplete`.
    1. `STRING|Activator` Object by name or label to receive this comm. Can also be a static world solar.
    1. `STRING` Object by name to send this comm. Can also be a static world solar, or `etherSender`. Must have a voice defined. Cannot be a player.
    1. `STRING` Multiple subsequent entries possible. Voice line to play. Must be defined for the voice.
    1. `[FLOAT] :0` The additional delay after this comm has ended before any other comm can reach the receiver. Also influences when the comm is considered complete.
    1. `[True|False] :False` Whether this comm can be heard by bystanders in space.

## `Trigger`

Triggers are the core logical elements of a mission. Multiple `Trigger` can be created for a mission. They always must contain a singular condition (`Cnd_`) which must be fulfilled to execute all actions (`Act_`). A condition of a trigger only can be fulfilled if the trigger is activated.

Only exactly one trigger is being processed at the same time. Triggers are being processed in the same order as they were activated.

- `nickname` The name of the trigger. Used as reference for following actions and for debug output.
    1. `STRING` The name. Must be unique within this mission.
    
- `[initstate]` Whether this trigger gets activated directly on mission start.
    1. `Active|Inactive :Inactive` The initial state.

- `[repeatable]` Allows the trigger to be executed more than once.
    1. `Off|Auto|Manual :Off` `Auto` instantly re-activates the trigger upon finished execution until being deactivated via `Act_DeactTrig`. `Manual` deactivates the trigger after finished execution until being re-activated via `Act_ActTrig`.

### Conditions

Only one condition must be present. If none is present, `Cnd_True` is used.

Conditions will always pick only _one arbitrary_ member of a label to fulfill. To process all members of a label, set `repeatable` for the `Trigger` and do remove processed members from this label.

The keyword `Stranger` is used to refer explicitely to all players not having a label assigned in this mission. It can be used in combination with action’s `Activator` to assign players to the mission.

- `Cnd_BaseEnter` Only players. Landing or logging in to a base. `Activator` will be the landed player.
    1. `STRING|Stranger` The players to await fully landing on a base.
    1. `[STRING]` Multiple subsequent entries possible. The base nickname the player landed on. If none is given, any base will count.

- `Cnd_BaseExit` Only players. Launching or logging out from a base. `Activator` will be the leaving player.
    1. `STRING|Stranger` The players leaving a base.
    1. `[STRING]` Multiple subsequent entries possible. The base nickname the player leaves from. If none is given, any base will count.

- `Cnd_Cloaked` Checks whether the object is fully cloaked or not. `Activator` will be the cloaked/uncloaked object.
    1. `STRING|Stranger` Object by name or label to watch cloaking state of.
    1. `True|False :False` `False` if the target must be uncloaked, or `True` if it must be cloaked.

- `Cnd_CommComplete` Waits for a comm to complete. See `Act_SendComm` and `Act_EtherComm`. `Activator` will be the sender of `Act_SendComm`, or nobody for `Act_EtherComm`.
    1. `STRING` The comm name to wait finishing for. Started comms not ending within 10 seconds will trigger `Cnd_CommComplete` to allow continuing the flow even if no player witnessed hearing it.

- `Cnd_Count` Counts the objects from a label. `Activator` will be nobody.
    1. `STRING` Objects by label to count.
    1. `INTEGER` The target count.
    1. `[Less|Equal|Greater] :Equal` The comparator against the target count.

- `Cnd_Destroyed` When something gets destroyed/despawned. `Activator` can be defined via the last argument.
    1. `STRING|Stranger` Object by name or label to await the destruction of.
    1. `[INTEGER] :0` The specific count of objects that must be destroyed to meet this condition. Any negative value will make this wait until all occurances of the objects are destroyed.
    1. `[Explode|Vanish|Any] :Any` Expects the destruction to be either by violence (`Explode`), despawn (e.g. by docking or explicit despawn) (`Vanish`) or whatever reason (`Any`).
    1. `[STRING]` Object by name or label that must be the killer.
    1. `[Killer|Destroyed] :Killer` Sets whether `Activator` will be the killer or the destroyed object. If set to `Killer` and the object is being despawned, nobody will be the activator.

- `Cnd_DistObj` Distance to another object in space. `Activator` will be the object coming into range. Evaluation will only happen if the target and destination objects are in space of the same system each.
    1. `STRING|Stranger` Must be a ship. Object by name or label to expect within the distance.
    1. `STRING` Other object by name or label to expect within the distance. Can also be a static world solar.
    1. `FLOAT :0` The distance from the given position to check.
    1. `[Inside|Outside] :Inside` Whether the objects must be within or outside this distance.

- `Cnd_DistVec` Distance from a vector in space. `Activator` will be the object coming into range. Evaluation will only happen if the object is in space of the target system.
    1. `STRING|Stranger` Must be a ship. Object by name or label to expect within the distance.
    1. `FLOAT :0` The x-axis position for this volume.
    1. `FLOAT :0` The y-axis position for this volume.
    1. `FLOAT :0` The z-axis position for this volume.
    1. `FLOAT :0` The distance from the given position to check.
    1. `STRING` The system nickname where the checks happen.
    1. `[Inside|Outside] :Inside` Whether the objects must be within or outside this distance.
    1. `[STRING]` The name of the hardpoint that will be used instead of the object’s center. Will be ignored if the hardpoint doesn’t exist.

- `Cnd_HasCargo` Only players. When the given cargo is in cargo hold. Does not count mounted items. **For now only evaluates on bases.**
    1. `STRING|Stranger` The players by label to check the cargo hold of.
    1. All cargo that must be present, defined in pairs of:
        1. `STRING` The cargo nickname to check for.
        1. `INTEGER :1` The minimum quantity of this cargo required.

- `Cnd_HealthDec` When the hitpoints falls below a threshold. `Activator` can be defined via the last argument.
    1. `STRING|Stranger` Object by name or label to observe the hitpoints of.
    1. `FLOAT :0` The percentage of hitpoints the target must fall below.
    1. `[Inflictor|Damaged] :Inflictor` Sets whether `Activator` will be the damage inflictor or the damaged object.
    1. `[Root|STRING] :Root` Multiple subsequent entries possible. Lists the target’s collision groups to check the hitpoint loss of.

- `Cnd_HealthInc` When the hitpoints raise above a threshold. `Activator` can be defined via the last argument. Does *not* register the usage of nanobots.
    1. `STRING|Stranger` Object by name or label to observe the hitpoints of.
    1. `FLOAT :0` The percentage of hitpoints the target must raise above.
    1. `[Inflictor|Repaired] :Inflictor` Sets whether `Activator` will be the repair inflictor or the repaired object.
    1. `[Root|STRING] :Root` Multiple subsequent entries possible. Lists the target’s collision groups to check the hitpoint gain of.

- `Cnd_InSpace` Only players. Checks whether the player is in space. `Activator` will be the player.
    1. `STRING|Stranger` The players by label to expect to be in space.
    1. `[STRING]` Multiple subsequent entries possible. The system nickname to expect the player in. If none is given, any system will count.

- `Cnd_InSystem` Only players. Checks whether the player is within a system. This can be either in space, or docked to a base. `Activator` will be the player.
    1. `STRING|Stranger` The players by label to expect to be in a system.
    1. `[STRING]` Multiple subsequent entries possible. The system nickname to expect the player in.

- `Cnd_InZone` Checks if an object is within a zone. `Activator` will be the object in the zone.
    1. `STRING|Stranger` Object by name or label to expect in the zone.
    1. `STRING` Multiple subsequent entries possible. The zone nickname to expect the object in.

- `Cnd_JumpInComplete` Only players. When fully jumped into a system. `Activator` will be the jumped-in player.
    1. `STRING|Stranger` The players by label to await fully jumping into a system.
    1. `[STRING]` Multiple subsequent entries possible. The system nickname the player jumped into. If none is given, any system will count.

- `Cnd_LaunchComplete` Only players. When fully launched from a base. `Activator` will be the launched player.
    1. `STRING|Stranger` The players by label to await fully landing on a base.
    1. `[STRING]` Multiple subsequent entries possible. The base nickname the player launched from. If none is given, any base will count.

- `Cnd_LeaveMsn` Only players. When leaving the mission by any means, including Mission Abort.
    1. `STRING|Any :Any` The players by label to await leaving. Matches any mission player if `Any` is given.

- `Cnd_OnBase` Only players. Checks if the player is on a base. `Activator` will be the player.
    1. `STRING|Stranger` The players by label to expect on a base.
    1. `[STRING]` Multiple subsequent entries possible. The base nickname the player must be on. If none is given, any base will count.

- `Cnd_ProjHitCount` Counts projectile hits to the target. `Activator` can be defined via the last argument.
    1. `STRING` Object by name or label to count projectile hits on.
    1. `[INTEGER] :1` The count of projectile hits that must have happened.
    1. `[Any|Hull|Shield] :Any` The surface the hit should be registered on.
    1. `[Any|Projectile|Explosion] :Any` The damage type to register. `Any` means `Projectile` and `Explosion`.
    1. `[STRING|Stranger]` Damage inflictor object by name or label. If none is given, defaults to all mission objects and players.
    1. `[Inflictor|Damaged] :Inflictor` Sets whether `Activator` will be the damage inflictor or the damaged object.

- `Cnd_SystemSpaceEnter` Only players. When entering space of a system. `Activator` will be the player.
    1. `STRING|Stranger` The players by label to await entering space.
    1. `[Jump|Launch|Spawn|Any] :Any` The condition under which space is entered.
    1. `[STRING]` Multiple subsequent entries possible. The system nickname the player entered. If none is given, any system will count.

- `Cnd_SystemSpaceExit` Only players. When leaving space of a system. `Activator` will be the player.
    1. `STRING|Stranger` The players by label to await leaving space.
    1. `[Jump|Dock|Explode|Vanish|Any] :Any` The condition under which space is left.
    1. `[STRING]` Multiple subsequent entries possible. The system nickname the player left. If none is given, any system will count.

- `Cnd_Timer` Waits until the time as passed. `Activator` will be nobody.
    1. `FLOAT` Lower limit of randomized time.
    1. `[FLOAT] :0` Upper limit of randomized time. When not set or lower than the lower limit, the timer will always use a constant time of lthe lower limit.

- `Cnd_True` No values. This instantly lets the trigger execute. `Activator` will be nobody.

### Actions

There can be as many actions as needed – even the same ones.

The keyword `Activator` is used to refer explicitely to the object/player that fulfilled the trigger condition. Sometimes this can be the server itself (e.g. `Cnd_True`, `Cnd_Timer`). The `Activator` can be used in combination with the condition’s `Stranger` to assign players to the mission or do other things only to non-assigned players.

- `Act_ActMsn` Activates another mission.
    1. `STRING` Mission nickname to refer.
    1. `[STRING|All]` Multiple subsequent entries possible. A label whose assigned players are transferred over to the mission with the same label. `All` transfers over all players and their labels.

- `Act_ActMsnTrig` Activates a trigger of a another mission. This works in two exclusive modes:
    - A single trigger with optional probability to be activated:
    1. `STRING` Mission nickname to refer.
    1. `STRING` Trigger nickname to refer.
    1. `[FLOAT] :1` A probability between `0` and `1` the trigger will be activated.
    - A list of triggers, each with a weight, to be picked randomly:
    1. `STRING` Mission nickname to refer.
    1. `STRING` Trigger nickname to refer.
    1. `[FLOAT] :1` The weighted chance for this trigger to be picked for activation.

- `Act_ActTrig` Activates a trigger. This works in two exclusive modes:
    - A single trigger with optional probability to be activated:
    1. `STRING` Trigger nickname to refer.
    1. `[FLOAT] :1` A probability between `0` and `1` the trigger will be activated.
    - A list of triggers, each with a weight, to be picked randomly:
    1. `STRING` Trigger nickname to refer.
    1. `[FLOAT] :1` The weighted chance for this trigger to be picked for activation.

- `Act_AddCargo` Only for players. Adds cargo. Will do nothing if player has not enough cargo space. **Doing this immediately after `Cnd_HasCargo` triggered by buying goods may trigger a cheat-detection kick.**
    1. `STRING|Activator` Players by label to receive the cargo.
    1. `STRING` The item nickname to use.
    1. `INTEGER :0` Amount of items to add to cargo.
    1. `[True|False :False]` Whether this cargo is flagged as mission item. Only works for Commodities. A mission item cannot be sold, traded to other players, or dropped into space. It will be deleted on death, or when a player switches character or leaves the server. It must be manually removed when leaving the mission, or on mission success or abortion.

- `Act_AddLabel` Adds a label to the objects. **This is the only way to assign players to the mission.**
    1. `STRING|Activator` Object by name or label to manipulate.
    1. `STRING` The label to add.

- `Act_AdjAcct` Only for players. Adjusts the cash on the account. Cash will be automatically clamped to prevent overflows/underflows.
    1. `STRING|Activator` Players by label to have their cash being modified.
    1. `INTEGER :0` A positive or negative number of cash. Cannot exceed more than +-(2^32)-1.
    1. `[True|False] :False` Split the cash across all player members of the receiving label equally. No effect if a singular player receives the cash.

- `Act_AdjRep` Only for players. Adjusts the reputation of the player toward a specific faction.
    1. `STRING|Activator` Players by label to have their reputation being modified.
    1. `STRING` The faction name to change reputation toward. Relative changes according to `empathy.ini` will be computed.
    1. `FLOAT|ObjectDestruction|MissionSuccess|MissionFailure|MissionAbortion :ObjectDestruction` The change magnitue. Either uses a given value, or takes one of the predefined events from `empathy.ini`.

- `Act_Cloak` Only for NPCs or solars. Cloaks or uncloaks them when a cloaking device is mounted.
    1. `STRING|Activator` Object by name or label for which the cloak will be changed.
    1. `True|False :False` Whether the cloak should be active or not.

- `Act_DeactMsnTrig` Deactivates a trigger of a another mission. This works in two exclusive modes:
    - A single trigger with optional probability to be deactivated:
    1. `STRING` Mission nickname to refer.
    1. `STRING` Trigger nickname to refer.
    1. `[FLOAT] :1` A probability between `0` and `1` the trigger will be deactivated.
    - A list of triggers, each with a weight, to be picked randomly:
    1. `STRING` Mission nickname to refer.
    1. `STRING` Trigger nickname to refer.
    1. `[FLOAT] :1` The weighted chance for this trigger to be picked for deactivation.

- `Act_DeactTrig` Deactivates a trigger. This works in two exclusive modes:
    - A single trigger with optional probability to be deactivated:
    1. `STRING` Trigger nickname to refer.
    1. `[FLOAT] :1` A probability between `0` and `1` the trigger will be deactivated.
    - A list of triggers, each with a weight, to be picked randomly:
    1. `STRING` Trigger nickname to refer.
    1. `[FLOAT] :1` The weighted chance for this trigger to be picked for deactivation.

- `Act_DebugMsg` Prints a message into Hook console and to all players registered to the mission.
    1. `STRING` Arbitrary text to print.

- `Act_Destroy` Destroys an object.
    1. `STRING|Activator` Object by name or label to destroy.
    1. `[Explode|Silent] :Silent` Whether to explode the object or despawn it. Explosion does *not* trigger the death fuse.

- `Act_DockInstant` Only for players. Forces docking instantly with the given target.
    1. `STRING|Activator` Players by label to be force-docked instantly.
    1. `STRING` Target object name or static solar to dock with.
    1. `STRING` Dock hardpoint name to dock at.

- `Act_Ethercomm` Sends communication from no specific source to others. A sender without proper space costume will not display a comms window.
    1. `STRING` The name of this comm. Referred to by `Cnd_CommComplete`.
    1. `STRING|Activator` Object by name or label to receive this comm.
    1. `STRING` The voice nickname to use for the sender.
    1. `STRING` Multiple subsequent entries possible. Voice line to play. Must be defined for the voice.
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

- `Act_GiveObjList` Gives NPCs a list of objectives.
    1. `STRING|Activator` The NPCs to receive the objectives.
    1. `STRING` The `ObjList` nickname to refer to.

- `Act_Invulnerable` Only for NPCs or solars. Sets invulnerability for the target.
    1. `STRING|Activator` Object by name or label for which invulnerability will be set.
    1. `True|False :False` Whether to prevent non-player damage (NPCs, radiation, asteroid mines) or not.
    1. `[True|False] :False` Whether to prevent player damage or not.
    1. `[FLOAT] :0` The percentage of hitpoints the target must lose before the damage prevention kicks in.

- `Act_LeaveMsn` Only for players. Removes members of the label from the mission.
    1. `STRING|Activator` Players by label to remove from the mission.
    1. `[Silent|Success|Failure] :Silent` The way the players leave the mission. `Success` and `Failure` will show respective texts and play music.
    1. `[INTEGER] :0` Only for `Failure`: The text ID to display.

- `Act_LightFuse` Executes an arbitary fuse.
    1. `STRING|Activator` Object by name or label to refer.
    1. `STRING` Fuse nickname to execute on the objects.
    1. `[FLOAT]` The time-offset between `0` and `1` to start the fuse from.
    1. `[FLOAT]` Overrides the fuse lifetime by this value.

- `Act_Mark` Only for players. Marks another object.
    1. `STRING|Activator` Players by label to mark/unmark the target for.
    1. `STRING` Object by name or label to mark or unmark.
    1. `True|False :False` Whether to mark (`True`) or unmark (`False`) the target.

- `Act_NNPath` Only for players. Sets their normal player waypoint route. For a waypoint the system and position must be given. It will clear all waypoints if the system is not specified.
    1. `STRING|Activator` Players by label to set the message or waypoint.
    1. `[INTEGER] :0` Resource ID to display as message to the players. `0` shows no message.
    1. `[STRING]` The system nickname for the waypoint.
    1. `[FLOAT] :0` The x-axis position for the waypoint.
    1. `[FLOAT] :0` The y-axis position for the waypoint.
    1. `[FLOAT] :0` The z-axis position for the waypoint.
    1. `[True|False] :False` Whether this should be not a singular waypoint but an actual best-path route. **Best route may not work if the player does not have relevant system connections discovered.**
    1. `[STRING]` The optional object nickname to specify as waypoint destination. Not limited to the mission; this can be any static world solar.
    
- `Act_PlayMusic` Only for players. Sets the music. This will remain until music is reset by all values being `Default`, player changes system, player docks, logs out from character.
    1. `STRING|Activator` Players by label to set music for.
    1. `[STRING|Default] :Default` Overrides the space music.
    1. `[STRING|Default] :Default` Overrides the danger music.
    1. `[STRING|Default] :Default` Overrides the battle music.
    1. `[STRING|None] :None` Overrides all music by this track.
    1. `[FLOAT] :0` The time in seconds it takes to transition music.
    1. `[True|False] :False` Whether to play the override music only once and then return to other music.

- `Act_PlaySoundEffect` Only for players. Plays a single sound effect. This is *not* audible for other players.
    1. `STRING|Activator` Players by label to play the sound effect for.
    1. `STRING` The sound nickname to play.

- `Act_RemoveCargo` Only for players. Removes cargo.
    1. `STRING|Activator` Players by label to have the cargo removed from.
    1. `STRING` The item nickname to use.
    1. `INTEGER :0` Amount of items to remove from cargo.

- `Act_RemoveLabel` Removes a label to the objects. **This is the only explicit way to unassign players from the mission.**
    1. `STRING|Activator` Object by name or label to manipulate.
    1. `STRING` The label to remove.

- `Act_Relocate` Relocates an object.
    1. `STRING|Activator` Object by name to relocate.
    1. `FLOAT :0` Position on x-axis.
    1. `FLOAT :0` Position on y-axis.
    1. `FLOAT :0` Position on z-axis.
    1. `[FLOAT]` Override for current x-axis rotation.
    1. `[FLOAT]` Override for current y-axis rotation.
    1. `[FLOAT]` Override for current z-axis rotation.

- `Act_SendComm` Sends communication from one object to others. A sender without proper space costume will not display a comms window. Note that players cannot receive such comms from objects that are not present at the client (e.g. NPCs outside their spawn/sync range).
    1. `STRING` The name of this comm. Referred to by `Cnd_CommComplete`.
    1. `STRING|Activator` Object by name or label to receive this comm. Can also be a static world solar.
    1. `STRING` Object by name to send this comm. Can also be a static world solar. Must have a voice defined. Cannot be a player.
    1. `STRING` Multiple subsequent entries possible. Voice line to play. Must be defined for the voice.
    1. `[FLOAT] :0` The additional delay after this comm has ended before any other comm can reach the receiver. Also influences when the comm is considered complete.
    1. `[True|False] :False` Whether this comm can be heard by bystanders in space.

- `Act_SetDockState` Manually sets the dock animation state. This stays until any ship’s dock maneuver resets the state respectively.
    1. `STRING|Activator` Object by name or label to set the dock animation state to.
    1. `STRING` Dock hardpoint name to execute the animation of.
    1. `[Opened|Closed]: Closed` Whether to open or close the dock.

- `Act_SetLifeTime` Only for NPCs or solars. Once the object comes out of range from all players, its life time will be counted down. The life time will be restored completely if any player comes back into range to the object. All NPCs of a formation will be despawned if one of their members is running out of life time.
    1. `STRING|Activator` Object by name or label to change life time of.
    1. `FLOAT :1` The life time in seconds. `-1` means infinite life time.

- `Act_SetMsnResult` Sets the mission result. Relevant for reoffering missions to the job board. See `Mission: offer_reoffer`.
    1. `Success|Failure :Failure` Sets the mission result.

- `Act_SetNNObj` Only for players. Sets their current objective. Intermediate objective (Launch from Base, Dock to Tradelane, etc.) will be generated automatically. For a waypoint the system and position must be given. It will clear all waypoints if the system is not specified.
    1. `STRING|Activator` Players by label to set the message or waypoint.
    1. `[INTEGER] :0` Resource ID to display as message to the players. `0` shows no message.
    1. `[STRING]` The system nickname for the waypoint.
    1. `[FLOAT] :0` The x-axis position for the waypoint.
    1. `[FLOAT] :0` The y-axis position for the waypoint.
    1. `[FLOAT] :0` The z-axis position for the waypoint.
    1. `[True|False] :False` Whether this should generate a best-path route or just a direct waypoint at the target. For players not in the target system, best-path route will always be used until reaching the target system.
    1. `[STRING]` The optional object nickname to specify as waypoint destination. Not limited to the mission; this can be any static world solar.

- `Act_SetVibe` Sets the vibe/attitude uni-directional between two targets. For label members that join later this action must be invoked again.
    1. `STRING|Activator` Object by name or label whose vibe will be set. Can also be a static world solar. For players this only works if the target is another player.
    1. `STRING|Activator` Object by name or label to change the vibe toward. Can also be a static world solar. For players it will automatically change their vibe in turn, too. If both arguments are players, it will stay uni-directional.
    1. `FLOAT :0` The reputation value between `-1` and `1` to set the vibe on.

- `Act_SpawnFormation` Spawns a formation of ships.
    1. `STRING` The `MsnFormation` nickname to spawn.
    1. `[STRING|no_ol] :no_ol` The initial `ObjList` to spawn with. `no_ol` for none.
    1. `[FLOAT] :0` Override for initial x-axis position.
    1. `[FLOAT] :0` Override for initial y-axis position.
    1. `[FLOAT] :0` Override for initial z-axis position.
    1. `[FLOAT] :0` Override for initial x-axis rotation.
    1. `[FLOAT] :0` Override for initial y-axis rotation.
    1. `[FLOAT] :0` Override for initial z-axis rotation.

- `Act_SpawnSolar` Spawns a solar. Only one instance of it can exist at the same time.
    1. `STRING` The `MsnSolar` nickname to spawn.

- `Act_SpawnShip` Spawns a ship. Only one instance of it can exist at the same time.
    1. `STRING` The `MsnNpc` nickname to spawn.
    1. `[STRING|no_ol] :no_ol` The initial `ObjList` to spawn with. `no_ol` for none.
    1. `[FLOAT] :0` Override for initial x-axis position.
    1. `[FLOAT] :0` Override for initial y-axis position.
    1. `[FLOAT] :0` Override for initial z-axis position.
    1. `[FLOAT] :0` Override for initial x-axis rotation.
    1. `[FLOAT] :0` Override for initial y-axis rotation.
    1. `[FLOAT] :0` Override for initial z-axis rotation.

- `Act_StartDialog` Starts a `Dialog`.
    1. `STRING` Name of the `Dialog` to play. `Activator` is forwarded to it.

- `Act_TerminateMsn` Terminates the mission and cleans up all spawned objects with infinite life time, waypoints, and music. Evaluates whether the mission is being reoffered to the mission board, depending on `Act_SetMsnResult`.