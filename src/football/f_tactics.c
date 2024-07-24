#include "football/football.h"
#include <raylib.h>
#include <raymath.h>

static TeamFormationPosition TeamFormations[TF_COUNT][10] = {
    // TF_433
    {{.name = "LB",
      .style_of_play = SOP_BALANCED,
      .supported_roles = {PR_SB},
      .default_position = {.x = -0.8f, .y = 0.1f}},
     {.name = "LB",
      .style_of_play = SOP_BALANCED,
      .supported_roles = {PR_SB},
      .default_position = {.x = 0.8f, .y = 0.1f}},
     {.name = "CB",
      .style_of_play = SOP_DEFENSIVE,
      .supported_roles = {PR_CB},
      .default_position = {.x = -0.2f, .y = 0}},
     {.name = "CB",
      .style_of_play = SOP_DEFENSIVE,
      .supported_roles = {PR_CB},
      .default_position = {.x = 0.2f, .y = 0}},
     {.name = "DMF",
      .style_of_play = SOP_BALANCED,
      .supported_roles = {PR_DMF},
      .default_position = {.x = 0, .y = 0.2f}},
     {.name = "CMF",
      .style_of_play = SOP_BALANCED,
      .supported_roles = {PR_CMF},
      .default_position = {.x = -0.3f, .y = 0.5f}},
     {.name = "CMF",
      .style_of_play = SOP_BALANCED,
      .supported_roles = {PR_CMF},
      .default_position = {.x = 0.3f, .y = 0.5f}},
     {.name = "LWF",
      .style_of_play = SOP_OFFENSIVE,
      .supported_roles = {PR_WF},
      .default_position = {.x = -0.7f, .y = 0.8f}},
     {.name = "RWF",
      .style_of_play = SOP_OFFENSIVE,
      .supported_roles = {PR_WF},
      .default_position = {.x = 0.7f, .y = 0.8f}},
     {.name = "CF",
      .style_of_play = SOP_BALANCED,
      .supported_roles = {PR_CF},
      .default_position = {.x = 0, .y = 0.9f}}}};

TeamFormationPosition *football_get_team_formation(TeamFormation formation) {
    return TeamFormations[formation];
}

TeamFormationPosition *football_get_team_formation_position(TeamFormation formation,
                                                            u32 position_idx) {
    return &TeamFormations[formation][position_idx];
}
