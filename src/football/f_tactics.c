#include "football.h"
#include <raylib.h>
#include <raymath.h>

static TeamFormationPosition TeamFormations[TF_COUNT][10] = {
    // TF_433
    {{.name = "LB",
      .style_of_play = SOP_BALANCED,
      .supported_roles = {PR_SB},
      .position = {{.x = -0.6f, .y = 0.2f}, {.x = -0.8f, .y = 0.3f}, {.x = -0.7f, .y = 0.6f}}},
     {.name = "LB",
      .style_of_play = SOP_BALANCED,
      .supported_roles = {PR_SB},
      .position = {{.x = 0.6f, .y = 0.2f}, {.x = 0.8f, .y = 0.3f}, {.x = 0.7f, .y = 0.6f}}},
     {.name = "CB",
      .style_of_play = SOP_DEFENSIVE,
      .supported_roles = {PR_CB},
      .position = {{.x = -0.2f, .y = 0.1f}, {.x = -0.2f, .y = 0.2f}, {.x = -0.2f, .y = 0.5f}}},
     {.name = "CB",
      .style_of_play = SOP_DEFENSIVE,
      .supported_roles = {PR_CB},
      .position = {{.x = 0.2f, .y = 0.1f}, {.x = 0.2f, .y = 0.2f}, {.x = 0.2f, .y = 0.5f}}},
     {.name = "DMF",
      .style_of_play = SOP_BALANCED,
      .supported_roles = {PR_DMF},
      .position = {{.x = 0, .y = 0.2f}, {.x = 0, .y = 0.4f}, {.x = 0, .y = 0.7f}}},
     {.name = "CMF",
      .style_of_play = SOP_BALANCED,
      .supported_roles = {PR_CMF},
      .position = {{.x = -0.3, .y = 0.3f}, {.x = -0.3f, .y = 0.5f}, {.x = -0.3f, .y = 0.8f}}},
     {.name = "CMF",
      .style_of_play = SOP_BALANCED,
      .supported_roles = {PR_CMF},
      .position = {{.x = 0.3, .y = 0.3f}, {.x = 0.3f, .y = 0.5f}, {.x = 0.3f, .y = 0.8f}}},
     {.name = "LWF",
      .style_of_play = SOP_OFFENSIVE,
      .supported_roles = {PR_WF},
      .position = {{.x = -0.7f, .y = 0.35f}, {.x = -0.7f, .y = 0.6f}, {.x = -0.7f, .y = 0.9f}}},
     {.name = "RWF",
      .style_of_play = SOP_OFFENSIVE,
      .supported_roles = {PR_WF},
      .position = {{.x = 0.7f, .y = 0.35f}, {.x = 0.7f, .y = 0.6f}, {.x = 0.7f, .y = 0.9f}}},
     {.name = "CF",
      .style_of_play = SOP_BALANCED,
      .supported_roles = {PR_CF},
      .position = {{.x = 0, .y = 0.5f}, {.x = 0, .y = 0.7f}, {.x = 0, .y = 0.9f}}}}};

TeamFormationPosition *football_get_team_formation(TeamFormation formation) {
    return TeamFormations[formation];
}

TeamFormationPosition *football_get_team_formation_position(TeamFormation formation,
                                                            u32 position_idx) {
    return &TeamFormations[formation][position_idx];
}
