#include "Pilots.h"

namespace Pilots
{
	static std::unordered_map<uint, pub::AI::Personality::EvadeDodgeUseStruct> evadeDodgeBlocks;
	static std::unordered_map<uint, pub::AI::Personality::EvadeBreakUseStruct> evadeBreakBlocks;
	static std::unordered_map<uint, pub::AI::Personality::BuzzHeadTowardUseStruct> buzzHeadTowardBlocks;
	static std::unordered_map<uint, pub::AI::Personality::BuzzPassByUseStruct> buzzPassByBlocks;
	static std::unordered_map<uint, pub::AI::Personality::TrailUseStruct> trailBlocks;
	static std::unordered_map<uint, pub::AI::Personality::StrafeUseStruct> strafeBlocks;
	static std::unordered_map<uint, pub::AI::Personality::EngineKillUseStruct> engineKillBlocks;
	static std::unordered_map<uint, pub::AI::Personality::RepairUseStruct> repairBlocks;
	static std::unordered_map<uint, pub::AI::Personality::GunUseStruct> gunBlocks;
	static std::unordered_map<uint, pub::AI::Personality::MineUseStruct> mineBlocks;
	static std::unordered_map<uint, pub::AI::Personality::MissileUseStruct> missileBlocks;
	static std::unordered_map<uint, pub::AI::Personality::DamageReactionStruct> damageReactionBlocks;
	static std::unordered_map<uint, pub::AI::Personality::MissileReactionStruct> missileReactionBlocks;
	static std::unordered_map<uint, pub::AI::Personality::CountermeasureUseStruct> countermeasureBlocks;
	static std::unordered_map<uint, pub::AI::Personality::FormationUseStruct> formationBlocks;
	static std::unordered_map<uint, pub::AI::Personality::JobStruct> jobBlocks;

	struct Pilot
	{
		uint evadeDodgeId = 0;
		uint evadeBreakId = 0;
		uint buzzHeadTowardId = 0;
		uint buzzPassById = 0;
		uint trailId = 0;
		uint strafeId = 0;
		uint engineKillId = 0;
		uint repairId = 0;
		uint gunId = 0;
		uint mineId = 0;
		uint missileId = 0;
		uint damageReactionId = 0;
		uint missileReactionId = 0;
		uint countermeasureId = 0;
		uint formationId = 0;
		uint jobId = 0;
	};
	static std::unordered_map<uint, Pilot> pilots;

	pub::AI::Personality GetPilot(const uint pilotId)
	{
		pub::AI::Personality personality;
		const auto pilotEntry = pilots.find(pilotId);
		if (pilotEntry == pilots.end())
			return personality;
		const auto& pilot = pilotEntry->second;
		if (const auto& block = evadeDodgeBlocks.find(pilot.evadeDodgeId); block != evadeDodgeBlocks.end())
			personality.EvadeDodgeUse = block->second;
		if (const auto& block = evadeBreakBlocks.find(pilot.evadeBreakId); block != evadeBreakBlocks.end())
			personality.EvadeBreakUse = block->second;
		if (const auto& block = buzzHeadTowardBlocks.find(pilot.buzzHeadTowardId); block != buzzHeadTowardBlocks.end())
			personality.BuzzHeadTowardUse = block->second;
		if (const auto& block = buzzPassByBlocks.find(pilot.buzzPassById); block != buzzPassByBlocks.end())
			personality.BuzzPassByUse = block->second;
		if (const auto& block = trailBlocks.find(pilot.trailId); block != trailBlocks.end())
			personality.TrailUse = block->second;
		if (const auto& block = strafeBlocks.find(pilot.strafeId); block != strafeBlocks.end())
			personality.StrafeUse = block->second;
		if (const auto& block = engineKillBlocks.find(pilot.engineKillId); block != engineKillBlocks.end())
			personality.EngineKillUse = block->second;
		if (const auto& block = repairBlocks.find(pilot.repairId); block != repairBlocks.end())
			personality.RepairUse = block->second;
		if (const auto& block = gunBlocks.find(pilot.gunId); block != gunBlocks.end())
			personality.GunUse = block->second;
		if (const auto& block = mineBlocks.find(pilot.mineId); block != mineBlocks.end())
			personality.MineUse = block->second;
		if (const auto& block = missileBlocks.find(pilot.missileId); block != missileBlocks.end())
			personality.MissileUse = block->second;
		if (const auto& block = damageReactionBlocks.find(pilot.damageReactionId); block != damageReactionBlocks.end())
			personality.DamageReaction = block->second;
		if (const auto& block = missileReactionBlocks.find(pilot.missileReactionId); block != missileReactionBlocks.end())
			personality.MissileReaction = block->second;
		if (const auto& block = countermeasureBlocks.find(pilot.countermeasureId); block != countermeasureBlocks.end())
			personality.CountermeasureUse = block->second;
		if (const auto& block = formationBlocks.find(pilot.formationId); block != formationBlocks.end())
			personality.FormationUse = block->second;
		if (const auto& block = jobBlocks.find(pilot.jobId); block != jobBlocks.end())
			personality.Job = block->second;
		return personality;
	}

	pub::AI::Personality GetPilotWithJob(const uint pilotId, const uint jobId)
	{
		auto personality = GetPilot(pilotId);
		if (const auto& block = jobBlocks.find(jobId); block != jobBlocks.end())
			personality.Job = block->second;
		return personality;
	}

	static uint CreateIdOrNull(const char* str)
	{
		return strlen(str) > 0 ? CreateID(str) : 0;
	}

	static void ReadFile(const std::string& filePath)
	{
		INI_Reader ini;
		if (ini.open(filePath.c_str(), false))
		{
			while (ini.read_header())
			{
				if (ini.is_header("Pilot"))
				{
					uint id = 0;
					Pilot block;
					while (ini.read_value())
					{
						if (ini.is_value("nickname"))
							id = CreateIdOrNull(ini.get_value_string(0));
						else if (ini.is_value("inherit"))
						{
							if (const auto& otherBlock = pilots.find(CreateIdOrNull(ini.get_value_string(0))); otherBlock != pilots.end())
								block = otherBlock->second;
						}
						else if (ini.is_value("evade_dodge_id"))
							block.evadeDodgeId = CreateIdOrNull(ini.get_value_string(0));
						else if (ini.is_value("evade_break_id"))
							block.evadeBreakId = CreateIdOrNull(ini.get_value_string(0));
						else if (ini.is_value("buzz_head_toward_id"))
							block.buzzHeadTowardId = CreateIdOrNull(ini.get_value_string(0));
						else if (ini.is_value("buzz_pass_by_id"))
							block.buzzPassById = CreateIdOrNull(ini.get_value_string(0));
						else if (ini.is_value("trail_id"))
							block.trailId = CreateIdOrNull(ini.get_value_string(0));
						else if (ini.is_value("strafe_id"))
							block.strafeId = CreateIdOrNull(ini.get_value_string(0));
						else if (ini.is_value("engine_kill_id"))
							block.engineKillId = CreateIdOrNull(ini.get_value_string(0));
						else if (ini.is_value("repair_id"))
							block.repairId = CreateIdOrNull(ini.get_value_string(0));
						else if (ini.is_value("gun_id"))
							block.gunId = CreateIdOrNull(ini.get_value_string(0));
						else if (ini.is_value("mine_id"))
							block.mineId = CreateIdOrNull(ini.get_value_string(0));
						else if (ini.is_value("missile_id"))
							block.missileId = CreateIdOrNull(ini.get_value_string(0));
						else if (ini.is_value("damage_reaction_id"))
							block.damageReactionId = CreateIdOrNull(ini.get_value_string(0));
						else if (ini.is_value("missile_reaction_id"))
							block.missileReactionId = CreateIdOrNull(ini.get_value_string(0));
						else if (ini.is_value("countermeasure_id"))
							block.countermeasureId = CreateIdOrNull(ini.get_value_string(0));
						else if (ini.is_value("formation_id"))
							block.formationId = CreateIdOrNull(ini.get_value_string(0));
						else if (ini.is_value("job_id"))
							block.jobId = CreateIdOrNull(ini.get_value_string(0));
					}
					if (id)
						pilots.insert({ id, block });
				}

				else if (ini.is_header("EvadeDodgeBlock"))
				{
					uint id = 0;
					pub::AI::Personality::EvadeDodgeUseStruct block;
					while (ini.read_value())
					{
						if (ini.is_value("nickname"))
							id = CreateIdOrNull(ini.get_value_string(0));
						else if (ini.is_value("inherit"))
						{
							if (const auto& otherBlock = evadeDodgeBlocks.find(CreateIdOrNull(ini.get_value_string(0))); otherBlock != evadeDodgeBlocks.end())
								block = otherBlock->second;
						}
						else if (ini.is_value("evade_activate_range"))
							block.evade_activate_range = ini.get_value_float(0);
						else if (ini.is_value("evade_dodge_style_weight"))
						{
							const std::string type = ToLower(ini.get_value_string(0));
							if (type == "waggle")
								block.evade_dodge_style_weight[0] = ini.get_value_float(1);
							else if (type == "waggle_random")
								block.evade_dodge_style_weight[1] = ini.get_value_float(1);
							else if (type == "slide")
								block.evade_dodge_style_weight[2] = ini.get_value_float(1);
							else if (type == "corkscrew")
								block.evade_dodge_style_weight[3] = ini.get_value_float(1);
						}
						else if (ini.is_value("evade_dodge_cone_angle"))
							block.evade_dodge_cone_angle = ini.get_value_float(0);
						else if (ini.is_value("evade_dodge_cone_angle_variance_percent"))
							block.evade_dodge_cone_angle_variance_percent = ini.get_value_float(0);
						else if (ini.is_value("evade_dodge_waggle_axis_cone_angle"))
							block.evade_dodge_waggle_axis_cone_angle = ini.get_value_float(0);
						else if (ini.is_value("evade_dodge_roll_angle"))
							block.evade_dodge_roll_angle = ini.get_value_float(0);
						else if (ini.is_value("evade_dodge_interval_time"))
							block.evade_dodge_interval_time = ini.get_value_float(0);
						else if (ini.is_value("evade_dodge_interval_time_variance_percent"))
							block.evade_dodge_interval_time_variance_percent = ini.get_value_float(0);
						else if (ini.is_value("evade_dodge_distance"))
							block.evade_dodge_distance = ini.get_value_float(0);
						else if (ini.is_value("evade_dodge_time"))
							block.evade_dodge_time = ini.get_value_float(0);
						else if (ini.is_value("evade_dodge_slide_throttle"))
							block.evade_dodge_slide_throttle = ini.get_value_float(0);
						else if (ini.is_value("evade_dodge_turn_throttle"))
							block.evade_dodge_turn_throttle = ini.get_value_float(0);
						else if (ini.is_value("evade_dodge_corkscrew_turn_throttle"))
							block.evade_dodge_corkscrew_turn_throttle = ini.get_value_float(0);
						else if (ini.is_value("evade_dodge_corkscrew_roll_throttle"))
							block.evade_dodge_corkscrew_roll_throttle = ini.get_value_float(0);
						else if (ini.is_value("evade_dodge_corkscrew_roll_flip_direction"))
							block.evade_dodge_corkscrew_roll_flip_direction = ini.get_value_bool(0);
						else if (ini.is_value("evade_dodge_direction_weight"))
						{
							const std::string type = ToLower(ini.get_value_string(0));
							if (type == "left")
								block.evade_dodge_direction_weight[0] = ini.get_value_float(1);
							else if (type == "right")
								block.evade_dodge_direction_weight[1] = ini.get_value_float(1);
							else if (type == "up")
								block.evade_dodge_direction_weight[2] = ini.get_value_float(1);
							else if (type == "down")
								block.evade_dodge_direction_weight[3] = ini.get_value_float(1);
						}
					}
					if (id)
						evadeDodgeBlocks.insert({ id, block });
				}

				else if (ini.is_header("EvadeBreakBlock"))
				{
					uint id = 0;
					pub::AI::Personality::EvadeBreakUseStruct block;
					while (ini.read_value())
					{
						if (ini.is_value("nickname"))
							id = CreateIdOrNull(ini.get_value_string(0));
						else if (ini.is_value("inherit"))
						{
							if (const auto& otherBlock = evadeBreakBlocks.find(CreateIdOrNull(ini.get_value_string(0))); otherBlock != evadeBreakBlocks.end())
								block = otherBlock->second;
						}
						else if (ini.is_value("evade_break_time"))
							block.evade_break_time = ini.get_value_float(0);
						else if (ini.is_value("evade_break_interval_time"))
							block.evade_break_interval_time = ini.get_value_float(0);
						else if (ini.is_value("evade_break_afterburner_delay"))
							block.evade_break_afterburner_delay = ini.get_value_float(0);
						else if (ini.is_value("evade_break_afterburner_delay_variance_percent"))
							block.evade_break_afterburner_delay_variance_percent = ini.get_value_float(0);
						else if (ini.is_value("evade_break_direction_weight"))
						{
							const std::string type = ToLower(ini.get_value_string(0));
							if (type == "left")
								block.evade_break_direction_weight[0] = ini.get_value_float(1);
							else if (type == "right")
								block.evade_break_direction_weight[1] = ini.get_value_float(1);
							else if (type == "up")
								block.evade_break_direction_weight[2] = ini.get_value_float(1);
							else if (type == "down")
								block.evade_break_direction_weight[3] = ini.get_value_float(1);
						}
						else if (ini.is_value("evade_break_roll_throttle"))
							block.evade_break_roll_throttle = ini.get_value_float(0);
						else if (ini.is_value("evade_break_turn_throttle"))
							block.evade_break_turn_throttle = ini.get_value_float(0);
						else if (ini.is_value("evade_break_style_weight"))
						{
							const std::string type = ToLower(ini.get_value_string(0));
							if (type == "sideways")
								block.evade_break_style_weight[0] = ini.get_value_float(1);
							else if (type == "outrun")
								block.evade_break_style_weight[1] = ini.get_value_float(1);
							else if (type == "reverse")
								block.evade_break_style_weight[2] = ini.get_value_float(1);
						}
						else if (ini.is_value("evade_break_attempt_reverse_time"))
							block.evade_break_attempt_reverse_time = ini.get_value_float(0);
						else if (ini.is_value("evade_break_reverse_distance"))
							block.evade_break_reverse_distance = ini.get_value_float(0);
					}
					if (id)
						evadeBreakBlocks.insert({ id, block });
				}

				else if (ini.is_header("BuzzHeadTowardBlock"))
				{
					uint id = 0;
					pub::AI::Personality::BuzzHeadTowardUseStruct block;
					while (ini.read_value())
					{
						if (ini.is_value("nickname"))
							id = CreateIdOrNull(ini.get_value_string(0));
						else if (ini.is_value("inherit"))
						{
							if (const auto& otherBlock = buzzHeadTowardBlocks.find(CreateIdOrNull(ini.get_value_string(0))); otherBlock != buzzHeadTowardBlocks.end())
								block = otherBlock->second;
						}
						else if (ini.is_value("buzz_max_time_to_head_away"))
							block.buzz_max_time_to_head_away = ini.get_value_float(0);
						else if (ini.is_value("buzz_head_toward_style_weight"))
						{
							const std::string type = ToLower(ini.get_value_string(0));
							if (type == "straight_to")
								block.buzz_head_toward_style_weight[0] = ini.get_value_float(1);
							else if (type == "slide")
								block.buzz_head_toward_style_weight[1] = ini.get_value_float(1);
							else if (type == "waggle")
								block.buzz_head_toward_style_weight[2] = ini.get_value_float(1);
						}
						else if (ini.is_value("buzz_min_distance_to_head_toward"))
							block.buzz_min_distance_to_head_toward = ini.get_value_float(0);
						else if (ini.is_value("buzz_min_distance_to_head_toward_variance_percent"))
							block.buzz_min_distance_to_head_toward_variance_percent = ini.get_value_float(0);
						else if (ini.is_value("buzz_head_toward_engine_throttle"))
							block.buzz_head_toward_engine_throttle = ini.get_value_float(0);
						else if (ini.is_value("buzz_head_toward_turn_throttle"))
							block.buzz_head_toward_turn_throttle = ini.get_value_float(0);
						else if (ini.is_value("buzz_head_toward_roll_throttle"))
							block.buzz_head_toward_roll_throttle = ini.get_value_float(0);
						else if (ini.is_value("buzz_head_toward_roll_flip_direction"))
							block.buzz_head_toward_roll_flip_direction = ini.get_value_bool(0);
						else if (ini.is_value("buzz_dodge_direction_weight"))
						{
							const std::string type = ToLower(ini.get_value_string(0));
							if (type == "left")
								block.buzz_dodge_direction_weight[0] = ini.get_value_float(1);
							else if (type == "right")
								block.buzz_dodge_direction_weight[1] = ini.get_value_float(1);
							else if (type == "up")
								block.buzz_dodge_direction_weight[2] = ini.get_value_float(1);
							else if (type == "down")
								block.buzz_dodge_direction_weight[3] = ini.get_value_float(1);
						}
						else if (ini.is_value("buzz_dodge_turn_throttle"))
							block.buzz_dodge_turn_throttle = ini.get_value_float(0);
						else if (ini.is_value("buzz_dodge_cone_angle"))
							block.buzz_dodge_cone_angle = ini.get_value_float(0);
						else if (ini.is_value("buzz_dodge_cone_angle_variance_percent"))
							block.buzz_dodge_cone_angle_variance_percent = ini.get_value_float(0);
						else if (ini.is_value("buzz_dodge_waggle_axis_cone_angle"))
							block.buzz_dodge_waggle_axis_cone_angle = ini.get_value_float(0);
						else if (ini.is_value("buzz_dodge_roll_angle"))
							block.buzz_dodge_roll_angle = ini.get_value_float(0);
						else if (ini.is_value("buzz_dodge_interval_time"))
							block.buzz_dodge_interval_time = ini.get_value_float(0);
						else if (ini.is_value("buzz_dodge_interval_time_variance_percent"))
							block.buzz_dodge_interval_time_variance_percent = ini.get_value_float(0);
						else if (ini.is_value("buzz_slide_throttle"))
							block.buzz_slide_throttle = ini.get_value_float(0);
						else if (ini.is_value("buzz_slide_interval_time"))
							block.buzz_slide_interval_time = ini.get_value_float(0);
						else if (ini.is_value("buzz_slide_interval_time_variance_percent"))
							block.buzz_slide_interval_time_variance_percent = ini.get_value_float(0);
					}
					if (id)
						buzzHeadTowardBlocks.insert({ id, block });
				}

				else if (ini.is_header("BuzzPassByBlock"))
				{
					uint id = 0;
					pub::AI::Personality::BuzzPassByUseStruct block;
					while (ini.read_value())
					{
						if (ini.is_value("nickname"))
							id = CreateIdOrNull(ini.get_value_string(0));
						else if (ini.is_value("inherit"))
						{
							if (const auto& otherBlock = buzzPassByBlocks.find(CreateIdOrNull(ini.get_value_string(0))); otherBlock != buzzPassByBlocks.end())
								block = otherBlock->second;
						}
						else if (ini.is_value("buzz_pass_by_style_weight"))
						{
							const std::string type = ToLower(ini.get_value_string(0));
							if (type == "straight_by")
								block.buzz_pass_by_style_weight[0] = ini.get_value_float(1);
							else if (type == "break_away")
								block.buzz_pass_by_style_weight[1] = ini.get_value_float(1);
							else if (type == "engine_kill")
								block.buzz_pass_by_style_weight[2] = ini.get_value_float(1);
						}
						else if (ini.is_value("buzz_distance_to_pass_by"))
							block.buzz_distance_to_pass_by = ini.get_value_float(0);
						else if (ini.is_value("buzz_pass_by_time"))
							block.buzz_pass_by_time = ini.get_value_float(0);
						else if (ini.is_value("buzz_drop_bomb_on_pass_by"))
							block.buzz_drop_bomb_on_pass_by = ini.get_value_bool(0);
						else if (ini.is_value("buzz_break_direction_weight"))
						{
							const std::string type = ToLower(ini.get_value_string(0));
							if (type == "left")
								block.buzz_break_direction_weight[0] = ini.get_value_float(1);
							else if (type == "right")
								block.buzz_break_direction_weight[1] = ini.get_value_float(1);
							else if (type == "up")
								block.buzz_break_direction_weight[2] = ini.get_value_float(1);
							else if (type == "down")
								block.buzz_break_direction_weight[3] = ini.get_value_float(1);
						}
						else if (ini.is_value("buzz_break_direction_cone_angle"))
							block.buzz_break_direction_cone_angle = ini.get_value_float(0);
						else if (ini.is_value("buzz_break_turn_throttle"))
							block.buzz_break_turn_throttle = ini.get_value_float(0);
						else if (ini.is_value("buzz_pass_by_roll_throttle"))
							block.buzz_pass_by_roll_throttle = ini.get_value_float(0);
					}
					if (id)
						buzzPassByBlocks.insert({ id, block });
				}

				else if (ini.is_header("TrailBlock"))
				{
					uint id = 0;
					pub::AI::Personality::TrailUseStruct block;
					while (ini.read_value())
					{
						if (ini.is_value("nickname"))
							id = CreateIdOrNull(ini.get_value_string(0));
						else if (ini.is_value("inherit"))
						{
							if (const auto& otherBlock = trailBlocks.find(CreateIdOrNull(ini.get_value_string(0))); otherBlock != trailBlocks.end())
								block = otherBlock->second;
						}
						else if (ini.is_value("trail_lock_cone_angle"))
							block.trail_lock_cone_angle = ini.get_value_float(0);
						else if (ini.is_value("trail_break_time"))
							block.trail_break_time = ini.get_value_float(0);
						else if (ini.is_value("trail_min_no_lock_time"))
							block.trail_min_no_lock_time = ini.get_value_float(0);
						else if (ini.is_value("trail_break_roll_throttle"))
							block.trail_break_roll_throttle = ini.get_value_float(0);
						else if (ini.is_value("trail_break_afterburner"))
							block.trail_break_afterburner = ini.get_value_bool(0);
						else if (ini.is_value("trail_max_turn_throttle"))
							block.trail_max_turn_throttle = ini.get_value_float(0);
						else if (ini.is_value("trail_distance"))
							block.trail_distance = ini.get_value_float(0);
					}
					if (id)
						trailBlocks.insert({ id, block });
				}

				else if (ini.is_header("StrafeBlock"))
				{
					uint id = 0;
					pub::AI::Personality::StrafeUseStruct block;
					while (ini.read_value())
					{
						if (ini.is_value("nickname"))
							id = CreateIdOrNull(ini.get_value_string(0));
						else if (ini.is_value("inherit"))
						{
							if (const auto& otherBlock = strafeBlocks.find(CreateIdOrNull(ini.get_value_string(0))); otherBlock != strafeBlocks.end())
								block = otherBlock->second;
						}
						else if (ini.is_value("strafe_run_away_distance"))
							block.strafe_run_away_distance = ini.get_value_float(0);
						else if (ini.is_value("strafe_attack_throttle"))
							block.strafe_attack_throttle = ini.get_value_float(0);
						else if (ini.is_value("strafe_turn_throttle"))
							block.strafe_turn_throttle = ini.get_value_float(0);
					}
					if (id)
						strafeBlocks.insert({ id, block });
				}

				else if (ini.is_header("EngineKillBlock"))
				{
					uint id = 0;
					pub::AI::Personality::EngineKillUseStruct block;
					while (ini.read_value())
					{
						if (ini.is_value("nickname"))
							id = CreateIdOrNull(ini.get_value_string(0));
						else if (ini.is_value("inherit"))
						{
							if (const auto& otherBlock = engineKillBlocks.find(CreateIdOrNull(ini.get_value_string(0))); otherBlock != engineKillBlocks.end())
								block = otherBlock->second;
						}
						else if (ini.is_value("engine_kill_search_time"))
							block.engine_kill_search_time = ini.get_value_float(0);
						else if (ini.is_value("engine_kill_face_time"))
							block.engine_kill_face_time = ini.get_value_float(0);
						else if (ini.is_value("engine_kill_use_afterburner"))
							block.engine_kill_use_afterburner = ini.get_value_float(0);
						else if (ini.is_value("engine_kill_afterburner_time"))
							block.engine_kill_afterburner_time = ini.get_value_float(0);
						else if (ini.is_value("engine_kill_max_target_distance"))
							block.engine_kill_max_target_distance = ini.get_value_float(0);
					}
					if (id)
						engineKillBlocks.insert({ id, block });
				}

				else if (ini.is_header("RepairBlock"))
				{
					uint id = 0;
					pub::AI::Personality::RepairUseStruct block;
					while (ini.read_value())
					{
						if (ini.is_value("nickname"))
							id = CreateIdOrNull(ini.get_value_string(0));
						else if (ini.is_value("inherit"))
						{
							if (const auto& otherBlock = repairBlocks.find(CreateIdOrNull(ini.get_value_string(0))); otherBlock != repairBlocks.end())
								block = otherBlock->second;
						}
						else if (ini.is_value("use_shield_repair_pre_delay"))
							block.use_shield_repair_pre_delay = ini.get_value_float(0);
						else if (ini.is_value("use_shield_repair_at_damage_percent"))
							block.use_shield_repair_at_damage_percent = ini.get_value_float(0);
						else if (ini.is_value("use_shield_repair_post_delay"))
							block.use_shield_repair_post_delay = ini.get_value_float(0);
						else if (ini.is_value("use_hull_repair_pre_delay"))
							block.use_hull_repair_pre_delay = ini.get_value_float(0);
						else if (ini.is_value("use_hull_repair_at_damage_percent"))
							block.use_hull_repair_at_damage_percent = ini.get_value_float(0);
						else if (ini.is_value("use_hull_repair_post_delay"))
							block.use_hull_repair_post_delay = ini.get_value_float(0);
					}
					if (id)
						repairBlocks.insert({ id, block });
				}

				else if (ini.is_header("GunBlock"))
				{
					uint id = 0;
					pub::AI::Personality::GunUseStruct block;
					while (ini.read_value())
					{
						if (ini.is_value("nickname"))
							id = CreateIdOrNull(ini.get_value_string(0));
						else if (ini.is_value("inherit"))
						{
							if (const auto& otherBlock = gunBlocks.find(CreateIdOrNull(ini.get_value_string(0))); otherBlock != gunBlocks.end())
								block = otherBlock->second;
						}
						else if (ini.is_value("fire_style"))
							block.fire_style = ToLower(ini.get_value_string(0)) == "single" ? 1 : 0;
						else if (ini.is_value("gun_fire_interval_time"))
							block.gun_fire_interval_time = ini.get_value_float(0);
						else if (ini.is_value("gun_fire_interval_variance_percent"))
							block.gun_fire_interval_variance_percent = ini.get_value_float(0);
						else if (ini.is_value("gun_fire_burst_interval_time"))
							block.gun_fire_burst_interval_time = ini.get_value_float(0);
						else if (ini.is_value("gun_fire_burst_interval_variance_percent"))
							block.gun_fire_burst_interval_variance_percent = ini.get_value_float(0);
						else if (ini.is_value("gun_fire_no_burst_interval_time"))
							block.gun_fire_no_burst_interval_time = ini.get_value_float(0);
						else if (ini.is_value("gun_fire_accuracy_cone_angle"))
							block.gun_fire_accuracy_cone_angle = ini.get_value_float(0);
						else if (ini.is_value("gun_fire_accuracy_power"))
							block.gun_fire_accuracy_power = ini.get_value_float(0);
						else if (ini.is_value("gun_fire_accuracy_power_npc"))
							block.gun_fire_accuracy_power_npc = ini.get_value_float(0);
						else if (ini.is_value("gun_range_threshold"))
							block.gun_range_threshold = ini.get_value_float(0);
						else if (ini.is_value("gun_range_threshold_variance_percent"))
							block.gun_range_threshold_variance_percent = ini.get_value_float(0);
						else if (ini.is_value("gun_target_point_switch_time"))
							block.gun_target_point_switch_time = ini.get_value_float(0);
						else if (ini.is_value("auto_turret_interval_time"))
							block.auto_turret_interval_time = ini.get_value_float(0);
						else if (ini.is_value("auto_turret_burst_interval_time"))
							block.auto_turret_burst_interval_time = ini.get_value_float(0);
						else if (ini.is_value("auto_turret_no_burst_interval_time"))
							block.auto_turret_no_burst_interval_time = ini.get_value_float(0);
						else if (ini.is_value("auto_turret_burst_interval_variance_percent"))
							block.auto_turret_burst_interval_variance_percent = ini.get_value_float(0);
					}
					if (id)
						gunBlocks.insert({ id, block });
				}

				else if (ini.is_header("MineBlock"))
				{
					uint id = 0;
					pub::AI::Personality::MineUseStruct block;
					while (ini.read_value())
					{
						if (ini.is_value("nickname"))
							id = CreateIdOrNull(ini.get_value_string(0));
						else if (ini.is_value("inherit"))
						{
							if (const auto& otherBlock = mineBlocks.find(CreateIdOrNull(ini.get_value_string(0))); otherBlock != mineBlocks.end())
								block = otherBlock->second;
						}
						else if (ini.is_value("mine_launch_interval"))
							block.mine_launch_interval = ini.get_value_float(0);
						else if (ini.is_value("mine_launch_cone_angle"))
							block.mine_launch_cone_angle = ini.get_value_float(0);
						else if (ini.is_value("mine_launch_range"))
							block.mine_launch_range = ini.get_value_float(0);
					}
					if (id)
						mineBlocks.insert({ id, block });
				}

				else if (ini.is_header("MissileBlock"))
				{
					uint id = 0;
					pub::AI::Personality::MissileUseStruct block;
					while (ini.read_value())
					{
						if (ini.is_value("nickname"))
							id = CreateIdOrNull(ini.get_value_string(0));
						else if (ini.is_value("inherit"))
						{
							if (const auto& otherBlock = missileBlocks.find(CreateIdOrNull(ini.get_value_string(0))); otherBlock != missileBlocks.end())
								block = otherBlock->second;
						}
						else if (ini.is_value("missile_launch_range"))
							block.missile_launch_range = ini.get_value_float(0);
						else if (ini.is_value("missile_launch_allow_out_of_range"))
							block.missile_launch_allow_out_of_range = ini.get_value_bool(0);
						else if (ini.is_value("missile_launch_interval_time"))
							block.missile_launch_interval_time = ini.get_value_float(0);
						else if (ini.is_value("missile_launch_interval_variance_percent"))
							block.missile_launch_interval_variance_percent = ini.get_value_float(0);
						else if (ini.is_value("missile_launch_cone_angle"))
							block.missile_launch_cone_angle = ini.get_value_float(0);
						else if (ini.is_value("anti_cruise_missile_min_distance"))
							block.anti_cruise_missile_min_distance = ini.get_value_float(0);
						else if (ini.is_value("anti_cruise_missile_max_distance"))
							block.anti_cruise_missile_max_distance = ini.get_value_float(0);
						else if (ini.is_value("anti_cruise_missile_pre_fire_delay"))
							block.anti_cruise_missile_pre_fire_delay = ini.get_value_float(0);
						else if (ini.is_value("anti_cruise_missile_interval_time"))
							block.anti_cruise_missile_interval_time = ini.get_value_float(0);
					}
					if (id)
						missileBlocks.insert({ id, block });
				}

				else if (ini.is_header("DamageReactionBlock"))
				{
					uint id = 0;
					pub::AI::Personality::DamageReactionStruct block;
					while (ini.read_value())
					{
						if (ini.is_value("nickname"))
							id = CreateIdOrNull(ini.get_value_string(0));
						else if (ini.is_value("inherit"))
						{
							if (const auto& otherBlock = damageReactionBlocks.find(CreateIdOrNull(ini.get_value_string(0))); otherBlock != damageReactionBlocks.end())
								block = otherBlock->second;
						}
						else if (ini.is_value("evade_break_damage_trigger_percent"))
							block.evade_break_damage_trigger_percent = ini.get_value_float(0);
						else if (ini.is_value("evade_dodge_more_damage_trigger_percent"))
							block.evade_dodge_more_damage_trigger_percent = ini.get_value_float(0);
						else if (ini.is_value("drop_mines_damage_trigger_percent"))
							block.drop_mines_damage_trigger_percent = ini.get_value_float(0);
						else if (ini.is_value("drop_mines_damage_trigger_time"))
							block.drop_mines_damage_trigger_time = ini.get_value_float(0);
						else if (ini.is_value("engine_kill_face_damage_trigger_percent"))
							block.engine_kill_face_damage_trigger_percent = ini.get_value_float(0);
						else if (ini.is_value("engine_kill_face_damage_trigger_time"))
							block.engine_kill_face_damage_trigger_time = ini.get_value_float(0);
						else if (ini.is_value("roll_damage_trigger_percent"))
							block.roll_damage_trigger_percent = ini.get_value_float(0);
						else if (ini.is_value("roll_damage_trigger_time"))
							block.roll_damage_trigger_time = ini.get_value_float(0);
						else if (ini.is_value("afterburner_damage_trigger_percent"))
							block.afterburner_damage_trigger_percent = ini.get_value_float(0);
						else if (ini.is_value("afterburner_damage_trigger_time"))
							block.afterburner_damage_trigger_time = ini.get_value_float(0);
						else if (ini.is_value("brake_reverse_damage_trigger_percent"))
							block.brake_reverse_damage_trigger_percent = ini.get_value_float(0);
						else if (ini.is_value("fire_missiles_damage_trigger_percent"))
							block.fire_missiles_damage_trigger_percent = ini.get_value_float(0);
						else if (ini.is_value("fire_missiles_damage_trigger_time"))
							block.fire_missiles_damage_trigger_time = ini.get_value_float(0);
						else if (ini.is_value("fire_guns_damage_trigger_percent"))
							block.fire_guns_damage_trigger_percent = ini.get_value_float(0);
						else if (ini.is_value("fire_guns_damage_trigger_time"))
							block.fire_guns_damage_trigger_time = ini.get_value_float(0);
					}
					if (id)
						damageReactionBlocks.insert({ id, block });
				}

				else if (ini.is_header("MissileReactionBlock"))
				{
					uint id = 0;
					pub::AI::Personality::MissileReactionStruct block;
					while (ini.read_value())
					{
						if (ini.is_value("nickname"))
							id = CreateIdOrNull(ini.get_value_string(0));
						else if (ini.is_value("inherit"))
						{
							if (const auto& otherBlock = missileReactionBlocks.find(CreateIdOrNull(ini.get_value_string(0))); otherBlock != missileReactionBlocks.end())
								block = otherBlock->second;
						}
						else if (ini.is_value("evade_break_missile_reaction_time"))
							block.evade_break_missile_reaction_time = ini.get_value_float(0);
						else if (ini.is_value("evade_slide_missile_reaction_time"))
							block.evade_slide_missile_reaction_time = ini.get_value_float(0);
						else if (ini.is_value("evade_afterburn_missile_reaction_time"))
							block.evade_afterburn_missile_reaction_time = ini.get_value_float(0);
						else if (ini.is_value("evade_missile_distance"))
							block.evade_missile_distance = ini.get_value_float(0);
					}
					if (id)
						missileReactionBlocks.insert({ id, block });
				}

				else if (ini.is_header("CountermeasureBlock"))
				{
					uint id = 0;
					pub::AI::Personality::CountermeasureUseStruct block;
					while (ini.read_value())
					{
						if (ini.is_value("nickname"))
							id = CreateIdOrNull(ini.get_value_string(0));
						else if (ini.is_value("inherit"))
						{
							if (const auto& otherBlock = countermeasureBlocks.find(CreateIdOrNull(ini.get_value_string(0))); otherBlock != countermeasureBlocks.end())
								block = otherBlock->second;
						}
						else if (ini.is_value("countermeasure_active_time"))
							block.countermeasure_active_time = ini.get_value_float(0);
						else if (ini.is_value("countermeasure_unactive_time"))
							block.countermeasure_unactive_time = ini.get_value_float(0);
					}
					if (id)
						countermeasureBlocks.insert({ id, block });
				}

				else if (ini.is_header("FormationBlock"))
				{
					uint id = 0;
					pub::AI::Personality::FormationUseStruct block;
					while (ini.read_value())
					{
						if (ini.is_value("nickname"))
							id = CreateIdOrNull(ini.get_value_string(0));
						else if (ini.is_value("inherit"))
						{
							if (const auto& otherBlock = formationBlocks.find(CreateIdOrNull(ini.get_value_string(0))); otherBlock != formationBlocks.end())
								block = otherBlock->second;
						}
						else if (ini.is_value("force_attack_formation_active_time"))
							block.force_attack_formation_active_time = ini.get_value_float(0);
						else if (ini.is_value("force_attack_formation_unactive_time"))
							block.force_attack_formation_unactive_time = ini.get_value_float(0);
						else if (ini.is_value("break_formation_damage_trigger_percent"))
							block.break_formation_damage_trigger_percent = ini.get_value_float(0);
						else if (ini.is_value("break_formation_damage_trigger_time"))
							block.break_formation_damage_trigger_time = ini.get_value_float(0);
						else if (ini.is_value("break_apart_formation_damage_trigger_percent"))
							block.break_apart_formation_damage_trigger_percent = ini.get_value_float(0);
						else if (ini.is_value("break_apart_formation_damage_trigger_time"))
							block.break_apart_formation_damage_trigger_time = ini.get_value_float(0);
						else if (ini.is_value("break_formation_missile_reaction_time"))
							block.break_formation_missile_reaction_time = ini.get_value_float(0);
						else if (ini.is_value("break_apart_formation_missile_reaction_time"))
							block.break_apart_formation_missile_reaction_time = ini.get_value_float(0);
						else if (ini.is_value("break_apart_formation_on_buzz_head_toward"))
							block.break_apart_formation_on_buzz_head_toward = ini.get_value_bool(0);
						else if (ini.is_value("break_formation_on_buzz_head_toward_time"))
							block.break_formation_on_buzz_head_toward_time = ini.get_value_float(0);
						else if (ini.is_value("regroup_formation_on_buzz_head_toward"))
							block.regroup_formation_on_buzz_head_toward = ini.get_value_bool(0);
						else if (ini.is_value("break_apart_formation_on_buzz_pass_by"))
							block.break_apart_formation_on_buzz_pass_by = ini.get_value_bool(0);
						else if (ini.is_value("break_formation_on_buzz_pass_by_time"))
							block.break_formation_on_buzz_pass_by_time = ini.get_value_float(0);
						else if (ini.is_value("regroup_formation_on_buzz_pass_by"))
							block.regroup_formation_on_buzz_pass_by = ini.get_value_bool(0);
						else if (ini.is_value("break_apart_formation_on_evade_dodge"))
							block.break_apart_formation_on_evade_dodge = ini.get_value_bool(0);
						else if (ini.is_value("break_formation_on_evade_dodge_time"))
							block.break_formation_on_evade_dodge_time = ini.get_value_float(0);
						else if (ini.is_value("regroup_formation_on_evade_dodge"))
							block.regroup_formation_on_evade_dodge = ini.get_value_bool(0);
						else if (ini.is_value("break_apart_formation_on_evade_break"))
							block.break_apart_formation_on_evade_break = ini.get_value_bool(0);
						else if (ini.is_value("break_formation_on_evade_break_time"))
							block.break_formation_on_evade_break_time = ini.get_value_float(0);
						else if (ini.is_value("regroup_formation_on_evade_break"))
							block.regroup_formation_on_evade_break = ini.get_value_bool(0);
						else if (ini.is_value("formation_exit_mode"))
						{
							const std::string type = ToLower(ini.get_value_string(0));
							if (type == "break_away_from_center")
								block.formation_exit_mode = 0;
							else if (type == "break_away_from_center_afterburner")
								block.formation_exit_mode = 1;
							else if (type == "brake_reverse")
								block.formation_exit_mode = 2;
							else if (type == "outrun")
								block.formation_exit_mode = 3;
							else
								block.formation_exit_mode = 5;
						}
						else if (ini.is_value("formation_exit_top_turn_break_away_throttle"))
							block.formation_exit_top_turn_break_away_throttle = ini.get_value_float(0);
						else if (ini.is_value("formation_exit_roll_outrun_throttle"))
							block.formation_exit_roll_outrun_throttle = ini.get_value_float(0);
						else if (ini.is_value("formation_exit_max_time"))
							block.formation_exit_max_time = ini.get_value_float(0);
						else if (ini.is_value("leader_makes_me_tougher"))
							block.leader_makes_me_tougher = ini.get_value_bool(0);
					}
					if (id)
						formationBlocks.insert({ id, block });
				}

				else if (ini.is_header("JobBlock"))
				{
					uint id = 0;
					pub::AI::Personality::JobStruct block;
					bool attackPreferenceFound = false;
					bool attackSubtargetOrderFound = false;
					while (ini.read_value())
					{
						if (ini.is_value("nickname"))
							id = CreateIdOrNull(ini.get_value_string(0));
						else if (ini.is_value("inherit"))
						{
							if (const auto& otherBlock = jobBlocks.find(CreateIdOrNull(ini.get_value_string(0))); otherBlock != jobBlocks.end())
								block = otherBlock->second;
						}
						else if (ini.is_value("scene_toughness_threshold"))
						{
							const std::string type = ToLower(ini.get_value_string(0));
							if (type == "easiest")
								block.scene_toughness_threshold = 0;
							else if (type == "easy")
								block.scene_toughness_threshold = 1;
							else if (type == "hard")
								block.scene_toughness_threshold = 3;
							else if (type == "hardest")
								block.scene_toughness_threshold = 4;
							else
								block.scene_toughness_threshold = 2;
						}
						else if (ini.is_value("target_toughness_preference"))
						{
							const std::string type = ToLower(ini.get_value_string(0));
							if (type == "easiest")
								block.target_toughness_preference = 0;
							else if (type == "easy")
								block.target_toughness_preference = 1;
							else if (type == "hard")
								block.target_toughness_preference = 3;
							else if (type == "hardest")
								block.target_toughness_preference = 4;
							else
								block.target_toughness_preference = 2;
						}
						else if (ini.is_value("attack_preference"))
						{
							const std::string type = ToLower(ini.get_value_string(0));
							byte typeOrd;
							if (type == "fighter")
								typeOrd = 0;
							else if (type == "freighter")
								typeOrd = 1;
							else if (type == "transport")
								typeOrd = 2;
							else if (type == "gunboat")
								typeOrd = 3;
							else if (type == "cruiser")
								typeOrd = 4;
							else if (type == "capital")
								typeOrd = 5;
							else if (type == "tradelane")
								typeOrd = 6;
							else if (type == "jumpgate")
								typeOrd = 7;
							else if (type == "weapons_platform")
								typeOrd = 8;
							else if (type == "destroyable_depot")
								typeOrd = 9;
							else if (type == "solar")
								typeOrd = 10;
							else
								typeOrd = 11;

							// Reset the list to "empty"
							if (!attackPreferenceFound)
							{
								attackPreferenceFound = true;
								block.attack_order[0].type = 12;
							}

							for (byte index = 0; index < 12; index++)
							{
								const bool endOfList = block.attack_order[index].type > 11;
								if (endOfList || block.attack_order[index].type == typeOrd)
								{
									block.attack_order[index].type = typeOrd;
									block.attack_order[index].distance = ini.get_value_float(1);
									block.attack_order[index].flag = 0;

									std::string flags = ToLower(ini.get_value_string(2));
									std::vector<std::string> splitFlags;
									size_t delimiterPos;
									while ((delimiterPos = flags.find('|')) != std::string::npos)
									{
										splitFlags.push_back(flags.substr(0, delimiterPos));
										flags.erase(0, delimiterPos + 1);
									}
									splitFlags.push_back(flags.substr(0, delimiterPos));

									for (auto& flag : splitFlags)
									{
										flag = Trim(flag);
										if (flag == "guns")
											block.attack_order[index].flag |= 1;
										else if (flag == "guided")
											block.attack_order[index].flag |= 2;
										else if (flag == "unguided")
											block.attack_order[index].flag |= 4;
										else if (flag == "torpedo")
											block.attack_order[index].flag |= 8;
									}

									if (endOfList)
										block.attack_order[index + 1].type = 12;
									break;
								}
							}
						}
						else if (ini.is_value("attack_subtarget_order"))
						{
							const std::string type = ToLower(ini.get_value_string(0));
							byte typeOrd;
							if (type == "guns")
								typeOrd = 0;
							else if (type == "turrets")
								typeOrd = 1;
							else if (type == "launchers")
								typeOrd = 2;
							else if (type == "towers")
								typeOrd = 3;
							else if (type == "engines")
								typeOrd = 4;
							else if (type == "hull")
								typeOrd = 5;
							else
								typeOrd = 6;

							// Reset the list to "empty"
							if (!attackSubtargetOrderFound)
							{
								attackSubtargetOrderFound = true;
								block.attack_subtarget_order[0] = 7;
							}

							for (byte index = 0; index < 7; index++)
							{
								const bool endOfList = block.attack_subtarget_order[index] > 6;
								if (endOfList || block.attack_subtarget_order[index] == typeOrd)
								{
									block.attack_subtarget_order[index] = typeOrd;
									if (endOfList)
										block.attack_subtarget_order[index + 1] = 7;
									break;
								}
							}
						}
						else if (ini.is_value("wait_for_leader_target"))
							block.wait_for_leader_target = ini.get_value_bool(0);
						else if (ini.is_value("maximum_leader_target_distance"))
							block.maximum_leader_target_distance = ini.get_value_float(0);
						else if (ini.is_value("field_targeting"))
						{
							const std::string type = ToLower(ini.get_value_string(0));
							if (type == "never")
								block.field_targeting = 0;
							else if (type == "low_density")
								block.field_targeting = 1;
							else if (type == "high_density")
								block.field_targeting = 2;
							else
								block.field_targeting = 3;
						}
						else if (ini.is_value("loot_preference"))
						{
							block.loot_preference = 0;
							std::string flags = ToLower(ini.get_value_string(2));
							size_t delimiterPos;
							while ((delimiterPos = flags.find('|')) != std::string::npos)
							{
								const std::string line = Trim(flags.substr(0, delimiterPos));
								if (line == "lt_commodities")
									block.loot_preference |= 1;
								else if (line == "lt_equipment")
									block.loot_preference |= 2;
								else if (line == "lt_potions")
									block.loot_preference |= 4;
								else if (line == "lt_all")
									block.loot_preference = 7;
								flags.erase(0, delimiterPos + 1);
							}
						}
						else if (ini.is_value("loot_flee_threshold"))
						{
							const std::string type = ToLower(ini.get_value_string(0));
							if (type == "easiest")
								block.loot_flee_threshold = 0;
							else if (type == "easy")
								block.loot_flee_threshold = 1;
							else if (type == "hard")
								block.loot_flee_threshold = 3;
							else if (type == "hardest")
								block.loot_flee_threshold = 4;
							else
								block.loot_flee_threshold = 2;
						}
						else if (ini.is_value("flee_scene_threat_style"))
						{
							const std::string type = ToLower(ini.get_value_string(0));
							if (type == "easiest")
								block.flee_scene_threat_style = 0;
							else if (type == "easy")
								block.flee_scene_threat_style = 1;
							else if (type == "equal")
								block.flee_scene_threat_style = 2;
							else if (type == "hardest")
								block.flee_scene_threat_style = 4;
							else
								block.flee_scene_threat_style = 3;
						}
						else if (ini.is_value("flee_when_hull_damaged_percent"))
							block.flee_when_hull_damaged_percent = ini.get_value_float(0);
						else if (ini.is_value("flee_when_leader_flees_style"))
							block.flee_when_leader_flees_style = ini.get_value_bool(0);
						else if (ini.is_value("flee_no_weapons_style"))
							block.flee_no_weapons_style = ini.get_value_bool(0);
						else if (ini.is_value("allow_player_targeting"))
							block.allow_player_targeting = ini.get_value_bool(0);
						else if (ini.is_value("force_attack_formation"))
						{
							block.force_attack_formation = ini.get_value_bool(0);
							block.force_attack_formation_used = ini.get_value_bool(0);
						}
					}
					if (id)
						jobBlocks.insert({ id, block });
				}
			}
			ini.close();
		}
	}

	void ReadFiles()
	{
		std::string dataPath = "..\\data";
		INI_Reader ini;
		if (ini.open("freelancer.ini", false))
		{
			while (ini.read_header())
			{
				if (ini.is_header("Freelancer"))
				{
					while (ini.read_value())
					{
						if (ini.is_value("data path"))
							dataPath = ini.get_value_string(0);
					}
				}
			}
			ini.close();
		}

		// Those file paths are hardcoded by Freelancer itself.
		ReadFile(dataPath + "\\missions\\pilots_population.ini");
		ReadFile(dataPath + "\\missions\\pilots_story.ini");
	}
}