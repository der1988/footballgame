#include "football.h"
#include <raylib.h>
#include <raymath.h>

struct team_formation_position team_formations[TF_COUNT][10] = {
	// TF_433
	{
		{
			.name = "LB",
			.style_of_play = SOP_BALANCED,
			.supported_roles = {PR_SB},
			.position =
				{
					{.x = 0.2f, .y = 0.1f},
					{.x = 0.1f, .y = 0.3f},
					{.x = 0.3f, .y = 0.65f},
				},
		},
		{
			.name = "LB",
			.style_of_play = SOP_BALANCED,
			.supported_roles = {PR_SB},
			.position =
				{
					{.x = 0.8f, .y = 0.1f},
					{.x = 0.9f, .y = 0.3f},
					{.x = 0.7f, .y = 0.65f},
				},
		},
		{
			.name = "CB",
			.style_of_play = SOP_DEFENSIVE,
			.supported_roles = {PR_CB},
			.position =
				{
					{.x = 0.4f, .y = 0.02f},
					{.x = 0.4f, .y = 0.2f},
					{.x = 0.4f, .y = 0.55f},
				},
		},
		{
			.name = "CB",
			.style_of_play = SOP_DEFENSIVE,
			.supported_roles = {PR_CB},
			.position =
				{
					{.x = 0.6f, .y = 0.02f},
					{.x = 0.6f, .y = 0.2f},
					{.x = 0.6f, .y = 0.55f},
				},
		},
		{
			.name = "DMF",
			.style_of_play = SOP_BALANCED,
			.supported_roles = {PR_DMF},
			.position =
				{
					{.x = 0.5f, .y = 0.15f},
					{.x = 0.5f, .y = 0.4f},
					{.x = 0.5f, .y = 0.75f},
				},
		},
		{
			.name = "CMF",
			.style_of_play = SOP_BALANCED,
			.supported_roles = {PR_CMF},
			.position =
				{
					{.x = 0.35f, .y = 0.25f},
					{.x = 0.35f, .y = 0.5f},
					{.x = 0.35f, .y = 0.85f},
				},
		},
		{
			.name = "CMF",
			.style_of_play = SOP_BALANCED,
			.supported_roles = {PR_CMF},
			.position = {{.x = 0.65f, .y = 0.25f},
						 {.x = 0.65f, .y = 0.5f},
						 {.x = 0.65f, .y = 0.85f}},
		},
		{
			.name = "LWF",
			.style_of_play = SOP_OFFENSIVE,
			.supported_roles = {PR_WF},
			.position =
				{
					{.x = 0.15f, .y = 0.35f},
					{.x = 0.15f, .y = 0.6f},
					{.x = 0.2f, .y = 0.92f},
				},
		},
		{
			.name = "RWF",
			.style_of_play = SOP_OFFENSIVE,
			.supported_roles = {PR_WF},
			.position =
				{
					{.x = 0.85f, .y = 0.35f},
					{.x = 0.85f, .y = 0.6f},
					{.x = 0.8f, .y = 0.92f},
				},
		},
		{
			.name = "CF",
			.style_of_play = SOP_BALANCED,
			.supported_roles = {PR_CF},
			.position =
				{
					{.x = 0.5f, .y = 0.45f},
					{.x = 0.5f, .y = 0.7f},
					{.x = 0.5f, .y = 0.95f},
				},
		},
	}};

struct team_formation_position *football_tactics_get_team_formation(enum team_formation formation)
{
	return team_formations[formation];
}

struct team_formation_position *
football_tactics_get_team_formation_position(enum team_formation formation, u32 position_idx)
{
	return &team_formations[formation][position_idx];
}
