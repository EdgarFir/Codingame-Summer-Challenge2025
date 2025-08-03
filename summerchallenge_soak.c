#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>

/**
 * Win the water fight by controlling the most territory, or out-soak your opponent!
 **/

# define TOP_WINGER 1
# define BOTTOM_WINGER 2
# define TOP_PLAYMAKER 3 
# define BOTTOM_PLAYMAKER 4
# define ALONE_PLAYMAKER 5
# define BOX_TO_BOX 6

typedef struct s_agent_data
{
	int player_id;
	int agent_id;
	int shoot_cooldown;
	int optimal_range;
	int soaking_power;
	int splash_bombs;
	int x;
	int y;
	int curr_cooldown;
	int curr_wetness;
	int curr_splash_bombs;
	int target_x;
	int target_y;
	int def_target_x;
	int def_target_y;
    int role;
	int died;
	struct s_agent_data *next;
}	t_agent_data;

typedef struct s_cover
{
	int x;
	int y;
	int tile_type;
}	t_cover;

typedef struct s_gamedata
{
	int my_id;
	int height;
	int width;
	int **map;
	int number_of_agents;
	int top_winger_died;
	int bottom_winger_died;
	int top_playmaker_died;
	int alone_playmaker_died;
	int bottom_playmaker_died;
}	t_gamedata;

void handle_playmaker_targets(t_gamedata game_data, t_agent_data *my_agent, int my_agents_count, t_agent_data *my_lst);
void handle_box_to_box_targets(t_gamedata game_data, t_agent_data *my_agent, int my_agents_count, t_agent_data *my_lst);


int is_on_target(int my_x, int target_x, int my_y, int target_y)
{
    return (my_x == target_x && my_y == target_y);
}

int calculate_manhattan_distance(int x1, int x2, int y1, int y2)
{
    return (abs(x1 - x2) + abs(y1 - y2));
}

void addback_node(t_agent_data **lst, t_agent_data *new)
{
    t_agent_data *curr;

    if (!(*lst))
    {
        *lst = new;
        return;
    }
    curr = *lst;
    while (curr->next)

        curr = curr->next;
    curr->next = new;
}

void add_agent_new_data(t_agent_data **lst, int agent_id, int player_id, int shoot_cooldown, int optimal_range, int soaking_power, int splash_bombs)
{
	t_agent_data *new;

	new = malloc(sizeof(t_agent_data));
	memset(new, 0, sizeof(t_agent_data));
	new->agent_id = agent_id;
	new->player_id = player_id;
	new->shoot_cooldown = shoot_cooldown;
	new->optimal_range = optimal_range;
	new->soaking_power = soaking_power;
	new->splash_bombs = splash_bombs;
	new->died = 0;
	new->next = NULL;
	addback_node(lst, new);
}

t_agent_data *get_agent_data(t_agent_data *lst, int agent_id, int role)
{
	t_agent_data *curr;

	curr = lst;
	while (curr)
	{
		if (agent_id)
		{
			if (curr->agent_id == agent_id)
				return (curr);
		}
		if (role)
		{
			if (curr->role == role)
				return (curr);
		}
		curr = curr->next;
	}
	return (NULL);
}

void add_agent_data(t_agent_data *lst, int agent_id, int x, int y, int curr_cooldown, int curr_splash_bombs, int wetness, int height, int width)
{
	t_agent_data *agent_data;

	agent_data = get_agent_data(lst, agent_id, 0);
	if (!agent_data)
		return ;
	agent_data->curr_cooldown = curr_cooldown;
	agent_data->curr_splash_bombs = curr_splash_bombs;
	agent_data->curr_wetness = wetness;
	agent_data->x = x;
	agent_data->y = y;
}

void check_if_anyone_died(t_agent_data **lst, t_gamedata game_data)
{
	t_agent_data *prev;
	t_agent_data *curr;
	t_agent_data *tmp;

	prev = NULL;
	curr = *lst;
	while (curr)
	{
		int agent_x = curr->x;
		int agent_y = curr->y;
		if ((curr->player_id != game_data.my_id && game_data.map[agent_y][agent_x] != -1) 
		|| (curr->player_id == game_data.my_id && game_data.map[agent_y][agent_x] != 3))
		{
			if (curr->role == BOTTOM_WINGER && curr->player_id)
				game_data.bottom_winger_died = 1;
			else if (curr->role == TOP_WINGER)
				game_data.top_winger_died = 1;
			else if (curr->role == BOTTOM_PLAYMAKER)
				game_data.bottom_playmaker_died = 1;
			else if (curr->role == TOP_PLAYMAKER)
				game_data.top_playmaker_died = 1;
			tmp = curr->next;
			fprintf(stderr, "agent_id died: %d\n", curr->agent_id);
			if (!prev)
			{
				*lst = (*lst)->next;
			}
			else if (!tmp)
				prev->next = NULL;
			else
				prev->next = tmp;
			free(curr);
			curr = tmp;
		}
		else
		{
			prev = curr;
			curr = curr->next;
		}
	}
}

t_agent_data *get_enemy_to_shoot(t_agent_data *my_agent, t_agent_data *my_enemies)
{
    if (my_agent->curr_cooldown)
        return (NULL);
    t_agent_data *best_enemy = NULL;
    int best_wetness = -1;
    int best_distance = -1;

    int my_x = my_agent->x;
    int my_y = my_agent->y;
    int max_range = my_agent->optimal_range * 2;

    t_agent_data *curr = my_enemies;

    while (curr)
    {
		if (curr->died == 0)
		{
			int man_dist = calculate_manhattan_distance(my_x, curr->x, my_y, curr->y);
			if (man_dist < max_range)
			{
				if (!best_enemy)
				{
					best_enemy = curr;
					best_wetness = curr->curr_wetness;
					best_distance = man_dist;
				}
				else
				{
					if (curr->curr_wetness > best_wetness || 
						curr->curr_wetness == best_wetness && man_dist < best_distance)
					{
						best_enemy = curr;
						best_wetness = curr->curr_wetness;
						best_distance = man_dist;
					}
				}
			}
		}
        curr = curr->next;
    }
    return (best_enemy);
}


int has_friends_in_splash_range(t_gamedata game_data, int enemy_x, int enemy_y, int man_dist)
{
	int start_x = enemy_x - 1, end_x = enemy_x + 1;
	int start_y = enemy_y - 1, end_y = enemy_y + 1;

	for (int i = start_y; i < end_y; i++)
	{
		for (int j = start_x; j < end_x; j++)
		{
			if ((i >= 0 && i < game_data.height) && (j >= 0 && j < game_data.width))
			{
				if (game_data.map[i][j] == 3)
					return (1);
			}
		}
	}
	return (0);
}

t_agent_data *get_enemy_to_splash(t_agent_data *my_agent, t_gamedata game_data, t_agent_data *my_enemies, int enemies_c)
{
	t_agent_data	*best_enemy_to_splash;
	t_agent_data	*curr;
	int				best_enemy_wetness;
	int				my_x;
	int				my_y;

	if (!my_enemies || !my_agent->curr_splash_bombs)
		return (NULL);
	best_enemy_to_splash = NULL;
	curr = my_enemies;
	my_x = my_agent->x;
	my_y = my_agent->y;
	int man_dist = 0;
    while (curr)
	{
		if (curr->died == 0)
		{
			int enemy_x = curr->x;
			int enemy_y = curr->y;
			int enemy_wetness = curr->curr_wetness;
			man_dist = calculate_manhattan_distance(my_x, enemy_x, my_y, enemy_y);
			if (man_dist >= 2 && man_dist <= 5 && !has_friends_in_splash_range(game_data, enemy_x, enemy_y, man_dist) && curr->curr_wetness < 85)
			{
				if (man_dist == 3)
				{
					my_agent->target_x = my_x - 1;
					my_agent->target_y = my_y;
				}
				if (!best_enemy_to_splash)
				{
					best_enemy_to_splash = curr;
					best_enemy_wetness = enemy_wetness;
				}
				else
				{
					if (enemy_wetness > best_enemy_wetness)
					{
						best_enemy_to_splash = curr;
						best_enemy_wetness = enemy_wetness;
					}
				}
			}
		}
		curr = curr->next;
	}
    return (best_enemy_to_splash);
}

int count_friends_in_map(int height, int width, int map[height][width], int map_side)
{
	int start_x, end_x;
	int start_y, end_y;
	int friends_count;

	// If map side == 1 count left side of the map else count right side
	if (map_side == 1)
		start_x = 0, end_x = width / 2;
	else
		start_x = width / 2, end_x = width;
	start_y = 0, end_y = height;
	friends_count = 0;
	for (int i = start_y; i < end_y; i++)
	{
		for (int j = start_x; j < end_x; j++)
		{
			if (map[i][j] == 3)
				friends_count++;
		}
	}
	return (friends_count);
}

int check_cover(int height, int width, int map[height][width], t_cover *best_cover, t_cover *tmp_cover, int my_y, int my_x)
{
	int start_x, end_x;
	int start_y, end_y;
	int enemies_found;

	start_x = tmp_cover->x + 1, end_x = width - 1;
	start_y = tmp_cover->y + 1, end_y = height - 1;
	enemies_found = 0;
	for (int i = start_y; i < end_y; i++)
	{
		for (int j = start_x; j < end_x; j++)
		{
			if (map[i][j] == -1 && tmp_cover->x - 1 == my_x)
			{
				enemies_found = 1;
				break ;
			}
		}
	}
	if (!enemies_found)
		return (0);
	if (best_cover->tile_type == -1 || (best_cover->tile_type > 0 && best_cover->tile_type < tmp_cover->tile_type))
	{
		best_cover->x = tmp_cover->x;
		best_cover->y = tmp_cover->y;
		best_cover->tile_type = tmp_cover->tile_type;
	}
	return (1);
}

int	has_good_cover(int height, int width, int map[height][width], int my_y, int my_x)
{
	t_cover best_cover;
	t_cover tmp_cover;
	int		already_find_a_cover;

	already_find_a_cover = 0;
	best_cover.x = -1;
	best_cover.y = -1;
	best_cover.tile_type = -1;
	if (my_x + 1 < width && (map[my_y + 1][my_x] == 1 || map[my_y + 1][my_x] == 2))
	{
		tmp_cover.x = my_x + 1;
		tmp_cover.y = my_y;
		tmp_cover.tile_type = map[my_y + 1][my_x];
		check_cover(height, width, map, &best_cover, &tmp_cover, my_y, my_x);
	}
	else if (my_x - 1 >= 0 && (map[my_y - 1][my_x] == 1 || map[my_y - 1][my_x] == 2))
	{
		tmp_cover.x = my_x - 1;
		tmp_cover.y = my_y;
		tmp_cover.tile_type = map[my_y - 1][my_x];
		check_cover(height, width, map, &best_cover, &tmp_cover, my_y, my_x);
	}
	else if (my_y + 1 < height && (map[my_y][my_x + 1] == 1 || map[my_y][my_x + 1] == 2))
	{
		tmp_cover.x = my_x;
		tmp_cover.y = my_y + 1;
		tmp_cover.tile_type = map[my_x][my_y + 1] == 1;
		check_cover(height, width, map, &best_cover, &tmp_cover, my_y, my_x);
	}
	else if (my_y - 1 >= 0 && (map[my_y][my_x - 1] == 1 || map[my_y][my_x - 1] == 2))
	{
		tmp_cover.x = my_x;
		tmp_cover.y = my_y - 1;
		tmp_cover.tile_type = map[my_y][my_x - 1] == 1;
		check_cover(height, width, map, &best_cover, &tmp_cover, my_y, my_x);
	}
	if (best_cover.tile_type == -1)
		return (0);
	return (1);
}

int seek_near_cover(t_agent_data *curr, int height, int width, int map[height][width])
{
	int my_y = curr->y;
	int my_x = curr->x;

    if (my_y + 1 < height && has_good_cover(height, width, map, my_y + 1, my_x))
	{
		curr->target_x = my_x;
		curr->target_y = my_y + 1;
		return (1);
	}
	if (my_y - 1 >= 0 && has_good_cover(height, width, map, my_y - 1, my_x))
	{
		curr->target_x = my_x;
		curr->target_y = my_y - 1;
		return (1);
	}
	if (my_x + 1 < width && has_good_cover(height, width, map, my_y, my_x + 1))
	{
		curr->target_x = my_x + 1;
		curr->target_y = my_y;
		return (1);
	}
	if (my_x - 1 >= 0 && has_good_cover(height, width, map, my_y, my_x - 1))
	{
		curr->target_x = my_x - 1;
		curr->target_y = my_y;
		return (1);
	}
	return (0);
}


void set_my_agents_role_by_y(t_agent_data *my_agents_lst, int my_agent_count)
{
    t_agent_data *arr[my_agent_count];
    t_agent_data *curr = my_agents_lst;
	
    int i = 0;
    while (curr && i < my_agent_count)
    {
        arr[i++] = curr;
        curr = curr->next;
    }
    for (int j = 0; j < my_agent_count - 1; j++)
    {
        for (int k = 0; k < my_agent_count - j - 1; k++)
        {
            if (arr[k]->y > arr[k + 1]->y)
            {
                t_agent_data *tmp = arr[k];
                arr[k] = arr[k + 1];
                arr[k + 1] = tmp;
            }
        }
    }
    for (int j = 0; j < my_agent_count; j++)
    {
        if (j == 0)
            arr[j]->role = TOP_WINGER; 
        else if (j == my_agent_count - 1)
            arr[j]->role = BOTTOM_WINGER;
        else if (j == my_agent_count / 2 && my_agent_count > 3)
            arr[j]->role = ALONE_PLAYMAKER;
		else if (j == my_agent_count / 2 && my_agent_count == 3)
			arr[j]->role = BOX_TO_BOX;
        else
        {
			if (my_agent_count == 4)
				arr[j]->role = ALONE_PLAYMAKER;
			else
			{
				if (j < my_agent_count / 2)
					arr[j]->role = TOP_PLAYMAKER;
				else
					arr[j]->role = BOTTOM_PLAYMAKER;
			}
        }
    }
}

void handle_wingers_targets(t_gamedata game_data, t_agent_data *my_agent, int my_agents_count, t_agent_data *my_lst)
{
	if (my_agent->role == TOP_WINGER && (my_agent->curr_wetness > 50 && game_data.top_playmaker_died == 1 || game_data.bottom_winger_died == 1))
	{
		my_agent->role = TOP_PLAYMAKER;
		handle_playmaker_targets(game_data, my_agent, my_agents_count, my_lst);
		game_data.top_playmaker_died = 0;
	}
	else if (my_agent->role == BOTTOM_WINGER && (my_agent->curr_wetness > 50 && game_data.bottom_playmaker_died == 1 || game_data.top_winger_died == 1 || game_data.alone_playmaker_died == 1))
	{
		my_agent->role = BOTTOM_PLAYMAKER;
		handle_playmaker_targets(game_data, my_agent, my_agents_count, my_lst);
		game_data.bottom_playmaker_died = 0;
	}
	else
	{
		if (my_agent->agent_id > game_data.number_of_agents / 2)
			my_agent->target_x = 0;
		else
			my_agent->target_x = game_data.width - 1;
		if (my_agent->role == TOP_WINGER)
			my_agent->target_y = 0;
		else
			my_agent->target_y = game_data.height - 1;
	}
}

void handle_playmaker_targets(t_gamedata game_data, t_agent_data *my_agent, int my_agents_count, t_agent_data *my_lst)
{
	if (game_data.top_winger_died == 1 && my_agent->role != BOTTOM_PLAYMAKER)
	{
		my_agent->role = TOP_WINGER;
		handle_wingers_targets(game_data, my_agent, my_agents_count, my_lst);
		game_data.top_winger_died = 0;
	}
	else if (game_data.bottom_winger_died == 1 && my_agent->role != TOP_PLAYMAKER)
	{
		my_agent->role = BOTTOM_WINGER;
		handle_wingers_targets(game_data, my_agent, my_agents_count, my_lst);
		game_data.bottom_winger_died = 0;
	}
	else
	{
		if (my_agent->role == TOP_PLAYMAKER || my_agent->role == BOTTOM_PLAYMAKER)
		{
			int role_to_search = (my_agent->role == TOP_PLAYMAKER) ? TOP_WINGER : BOTTOM_WINGER;
			t_agent_data *winger = get_agent_data(my_lst, 0, role_to_search);
			if (winger)
			{
				int man_distance = calculate_manhattan_distance(my_agent->x, winger->x, my_agent->y, winger->y);
				if (man_distance < 2)
				{
					if (my_agent->y > winger->y && game_data.map[my_agent->y - 1][my_agent->x] == 0)
					{
						my_agent->target_y = my_agent->y - 1;
						my_agent->target_x = my_agent->x; 
					}
					else if (my_agent->y < winger->y && game_data.map[my_agent->y + 1][my_agent->x] == 0)
					{
						my_agent->target_y = my_agent->y + 1;
						my_agent->target_x = my_agent->x;
					}
					else if (my_agent->x > winger->x && game_data.map[my_agent->y][my_agent->x - 1] == 0)
					{
						my_agent->target_y = my_agent->y;
						my_agent->target_x = my_agent->x - 1;
					}
					else if (my_agent->x > winger->x && game_data.map[my_agent->y][my_agent->x + 1] == 0)
					{
						my_agent->target_y = my_agent->y;
						my_agent->target_x = my_agent->x + 1;
					}
				}
				else
				{
					my_agent->target_x = winger->x;
					my_agent->target_y = winger->y;
				}
			}
		}
		else // Alone playmaker
		{
			// Check left winger and right_winger wetness, if left winger is wetter than right_winger, 
			// change targers to left winger, else change targets ro right winger
			// Maybe later, optimize this one to check which side has more enemies, but by now lets check this
			t_agent_data *bot_winger = get_agent_data(my_lst, 0, BOTTOM_WINGER);
			t_agent_data *top_winger = get_agent_data(my_agent, 0, TOP_WINGER);

            int help_wingers = 0;
			if (my_agent->agent_id > game_data.number_of_agents / 2)
				my_agent->target_x = 0;
			else
				my_agent->target_x = game_data.width - 1;
			my_agent->target_y = game_data.height / 2;
			int bot_winger_wet = 0;
			int top_winger_wet = 0;
			if (bot_winger)
				bot_winger_wet = bot_winger->curr_wetness;
			if (top_winger)
				top_winger_wet = top_winger->curr_wetness;
			if (bot_winger_wet > top_winger_wet)
			{
				my_agent->target_x = bot_winger->x;
				my_agent->target_y = bot_winger->y;
			}
			else if (bot_winger_wet < top_winger_wet)
			{
				my_agent->target_x = top_winger->x;
				my_agent->target_y = top_winger->y;
			}
		}
	}
}

void handle_box_to_box_targets(t_gamedata game_data, t_agent_data *my_agent, int my_agents_count, t_agent_data *my_lst)
{
	if (game_data.top_winger_died == 1)
	{
		if (my_agents_count == 3)
			my_agent->role = TOP_WINGER;
		else if (my_agents_count == 4)
			my_agent->role = ALONE_PLAYMAKER;
		else
			my_agent->role = BOTTOM_PLAYMAKER;
		game_data.top_playmaker_died = 0;
		handle_playmaker_targets(game_data, my_agent, my_agents_count, my_lst);
	}
	else if (game_data.bottom_playmaker_died)
	{
		if (my_agents_count == 3)
			my_agent->role = BOTTOM_WINGER;
		else if (my_agents_count == 4)
			my_agent->role = ALONE_PLAYMAKER;
		else
			my_agent->role = BOTTOM_PLAYMAKER;
		game_data.bottom_playmaker_died = 0;
		handle_playmaker_targets(game_data, my_agent, my_agents_count, my_lst);
	}
	else
	{
		int start_x = (game_data.width / 2) / 2, end_x = (game_data.width / 2);
		int start_y = (game_data.height / 2) / 2, end_y = (game_data.height / 2) + ((game_data.height / 2) / 2);
		int sides_covered;
		int best_x_cover = -1;
		int best_y_cover = -1;
		int best_type_cover = -1;
		int best_sides_covered = -1;

		for (int i = start_y; i < end_y; i++)
		{
			sides_covered = 0;
			for (int j = start_x; j < end_x; j++)
			{
				if ((game_data.map[i][j] == 0 && (game_data.map[i + 1][j] == 1 || game_data.map[i + 1][j] == 2)) 
				|| (game_data.map[i][j] == 0 && (game_data.map[i - 1][j] == 1 || game_data.map[i - 1][j] == 2)))
				{
					sides_covered++;
					if (game_data.map[i][j + 1] == 1 || game_data.map[i][j + 1] == 2)
						sides_covered++;
					if (game_data.map[i][j - 1] == 1 || game_data.map[i][j - 1] == 2)
						sides_covered++;
					if (best_type_cover == -1)
					{
						best_y_cover = i;
						best_x_cover = j;
						best_type_cover = game_data.map[i + 1][j]; 
						best_sides_covered = sides_covered;
					}
					else
					{
						if (game_data.map[i + 1][j] > best_type_cover)
						{
							best_y_cover = i;
							best_x_cover = j;
							best_type_cover = game_data.map[i + 1][j];
							best_sides_covered = sides_covered;
						}
						else if (game_data.map[i + 1][j] == best_type_cover && sides_covered > best_sides_covered)
						{
							best_y_cover = i;
							best_x_cover = j;
							best_type_cover = game_data.map[i + 1][j];
							best_sides_covered = sides_covered;
						}
					}
				}
			}
		}
		if (best_type_cover == -1)
		{
			my_agent->target_x = start_y;
			my_agent->target_y = start_x;
		}
		else
		{
			my_agent->target_x = best_x_cover;
			my_agent->target_y = best_y_cover;
		}
	}
}

void update_my_agent_targets(t_gamedata game_data, t_agent_data *my_agent, int my_agents_count, t_agent_data *my_lst)
{
	if (my_agent->role == BOX_TO_BOX)
		handle_box_to_box_targets(game_data, my_agent, my_agents_count, my_lst);
	else if (my_agent->role >= TOP_PLAYMAKER && my_agent->role <= ALONE_PLAYMAKER)
		handle_playmaker_targets(game_data, my_agent, my_agents_count, my_lst);
	else if (my_agent->role == TOP_WINGER || my_agent->role == BOTTOM_WINGER)
		handle_wingers_targets(game_data, my_agent, my_agents_count, my_lst);
}

t_agent_data *get_most_wet_enemy(t_agent_data *enemies_lst)
{
	t_agent_data *curr;
	t_agent_data *most_wet = NULL;

	curr = enemies_lst;
	while (curr)
	{
		if (!most_wet)
			most_wet = curr;
		else
		{
			if (curr->curr_wetness > most_wet->curr_wetness)
				most_wet = curr;
		}
		curr = curr->next;
	}
	return (most_wet);
}

int	seek_cardeals_pos(t_gamedata game_data, t_agent_data *agent, int seek_covers)
{
	int my_x = agent->x, my_y = agent->y;
	int my_x_target = agent->target_x, my_y_target = agent->target_y;

	if (seek_covers)
	{
		// check up
		if (my_y - 1 >= 0)
		{
			if (game_data.map[my_y - 1][my_x] == 1 || game_data.map[my_y - 1][my_x] == 2)
				return (1);
		}
		// check down
		if (my_y + 1 < game_data.height)
		{
			if (game_data.map[my_y + 1][my_x] == 1 || game_data.map[my_y + 1][my_x] == 2)
				return (1);
		}
		// check left if is in target way
		if (my_x_target < game_data.width - 1 && my_x - 1 >= 0)
		{
			if (game_data.map[my_y][my_x - 1] == 1 || game_data.map[my_y][my_x - 1] == 2)
				return (1);
		}
		// check_right if is in target way
		if (my_x_target > 0 && my_x + 1 < game_data.width)
		{
			if (game_data.map[my_y][my_x + 1] == 1 || game_data.map[my_y][my_x + 1] == 2)
				return (1);
		}
	}
	return (0);
}

int agent_is_in_splash_range_from_enemy(int agent_x, int agent_y, t_agent_data *enemies_lst)
{
	t_agent_data *curr = enemies_lst;

	while (curr)
	{
		int man_dist = calculate_manhattan_distance(agent_x, curr->x, agent_y, curr->y);
		if (man_dist <= 8 && curr->curr_splash_bombs > 1)
			return (1);
		curr = curr->next;
	}
	return (0);
}

int main()
{
	// Main variables to create.
	t_agent_data *friends_lst = NULL;
	t_agent_data *enemies_lst = NULL;
	t_gamedata game_data;
	memset(&game_data, 0, sizeof(t_gamedata));

	/* ------ MAIN INFO ----------- */
    int my_id;
    scanf("%d", &my_id);
	game_data.my_id = my_id;
    int agent_data_count;
    int my_agent_init_count = 0;
    scanf("%d", &agent_data_count);
	game_data.number_of_agents = agent_data_count;
    for (int i = 0; i < agent_data_count; i++) {
        int agent_id, player, shoot_cooldown, optimal_range, soaking_power, splash_bombs;
        scanf("%d%d%d%d%d%d", &agent_id, &player, &shoot_cooldown, &optimal_range, &soaking_power, &splash_bombs);
		if (player == my_id)
        {
            add_agent_new_data(&friends_lst, agent_id, player, shoot_cooldown, optimal_range, soaking_power, splash_bombs);
            my_agent_init_count++;
        }
		else
			add_agent_new_data(&enemies_lst, agent_id, player, shoot_cooldown, optimal_range, soaking_power, splash_bombs);
    }
	int start_left_side = 0;
	if (friends_lst->agent_id < agent_data_count / 2)
		start_left_side = 1;
	/* ------------ MAP CREATION -----------*/
    int width;
    int height;
    scanf("%d%d", &width, &height);
	game_data.width = width;
	game_data.height = height;
	// Allocate map height * width
	game_data.map = malloc(sizeof(int *) * height);
	for (int i = 0; i < height; i++)
		game_data.map[i] = malloc(sizeof(int) * width);
    for (int i = 0; i < height; i++) {
        for (int j = 0; j < width; j++) {
            int x, y, tile_type;
            scanf("%d%d%d", &x, &y, &tile_type);
			game_data.map[y][x] = tile_type;
        }
    }
	/* ----------- GAME START --------- */
    // game loop
	int turns = 0;
    while (1) {
		// Clean map
		for (int i = 0; i < height; i++)
        {
            for (int j = 0; j < width; j++)
            {
                if (game_data.map[i][j] == -1 || game_data.map[i][j] == 3)
                    game_data.map[i][j] = 0;
            }
        }
		/*  -------------- GET IN GAME AGENT DATA INFO ---------------- */
        int agent_count;
        scanf("%d", &agent_count);
		t_agent_data *curr_my_agent = friends_lst;
		t_agent_data *curr_my_enemy = enemies_lst;
        for (int i = 0; i < agent_count; i++) {
            int agent_id, x, y, cooldown, splash_bombs, wetness;
            scanf("%d%d%d%d%d%d", &agent_id, &x, &y, &cooldown, &splash_bombs, &wetness);
			if (agent_id > agent_data_count / 2)
			{
				if (start_left_side)
				{
					add_agent_data(enemies_lst, agent_id, x, y, cooldown, splash_bombs, wetness, height, width);
					game_data.map[y][x] = -1;
				}
				else
				{
					add_agent_data(friends_lst, agent_id, x, y, cooldown, splash_bombs, wetness, height, width);
					game_data.map[y][x] = 3;
					curr_my_agent = curr_my_agent->next;
				}
			}
			else
			{
				if (start_left_side)
				{
					add_agent_data(friends_lst, agent_id, x, y, cooldown, splash_bombs, wetness, height, width);
					game_data.map[y][x] = 3;
				}
				else
				{
					add_agent_data(enemies_lst, agent_id, x, y, cooldown, splash_bombs, wetness, height, width);
					game_data.map[y][x] = -1;
				}
			}
        }
		// Number of alive agents controlled by you
		int my_agent_count;
        scanf("%d", &my_agent_count);
		int enemies_count = agent_count - my_agent_count;
		int attack_mode = 0;
		int attack_x_target = 0;
		int attack_y_target = 0;
		if (turns == 0)
		{
			set_my_agents_role_by_y(friends_lst, my_agent_init_count);
			t_agent_data *curr = friends_lst;
			while (curr)
			{
				update_my_agent_targets(game_data, curr, my_agent_count, friends_lst);
				curr = curr->next;
			}
		}
		else
		{
			check_if_anyone_died(&friends_lst, game_data);
			check_if_anyone_died(&enemies_lst, game_data);
			if (enemies_count == 1 || my_agent_count == 1 || enemies_count != my_agent_count)
			{
				attack_mode = 1;
				t_agent_data *most_wet_enemy = get_most_wet_enemy(enemies_lst);
				attack_x_target = most_wet_enemy->x;
				attack_y_target = most_wet_enemy->y;
			}	
			else
			{
				t_agent_data *curr = friends_lst;
				while (curr)
				{
					update_my_agent_targets(game_data, curr, my_agent_count, friends_lst);
					curr = curr->next;
				}
			}
		}
		turns++; 
		fprintf(stderr, "TURNS: %d\n", turns);
		t_agent_data *curr = friends_lst;
        for (int i = 0; i < my_agent_count; i++) {

			fprintf(stderr, "AGENT_ID: %d, ATTACK_MODE: %d, ROLE: %d\n", curr->agent_id, attack_mode, curr->role);
			t_agent_data *enemy_to_shoot = get_enemy_to_shoot(curr, enemies_lst);
			t_agent_data *enemy_to_splash = get_enemy_to_splash(curr, game_data, enemies_lst, 0);
			// Fix agent pos after target been set
			fprintf(stderr, "x: %d, target_x: %d, y: %d, target_y: %d\n", curr->x, curr->target_x, curr->y, curr->target_y);
			if (curr->x + 1 < width && game_data.map[curr->y][curr->x + 1] != 0)
			{
				if (curr->x < curr->target_x && curr->x + 1 < width && game_data.map[curr->y][curr->x + 1] == 0)
				{
					curr->target_y = curr->y;
					curr->target_x = curr->x + 1;
				}
				else if (curr->x > curr->target_x && curr->x - 1 >= 0 && game_data.map[curr->y][curr->x - 1] == 0)
				{
					curr->target_y = curr->y;
					curr->target_x = curr->x - 1;
				}
				if (curr->y < curr->target_y && curr->y + 1 < height && game_data.map[curr->y + 1][curr->x] == 0)
				{
					curr->target_y = curr->y + 1;
					curr->target_x = curr->x;
				}
				else if (curr->y > curr->target_y && curr->y - 1 >= 0 && game_data.map[curr->y - 1][curr->x] == 0)
				{
					curr->target_y = curr->y - 1;
					curr->target_x = curr->x;
				}
				else if (!seek_cardeals_pos(game_data, curr, 1))
				{
					if (curr->target_y < curr->y && curr->y - 1 >= 0)
					{
						if (game_data.map[curr->y - 1][curr->x] == 0)
						{
							curr->target_x = curr->x;
							curr->target_y = curr->y - 1;
						}
					}
					else if (curr->target_y > curr->y && curr->y + 1 < height)
					{
						if (game_data.map[curr->y + 1][curr->x] == 0)
						{
							curr->target_x = curr->x;
							curr->target_y = curr->y + 1;
						}
					}
				}
			}
			else
			{
				if (curr->role == BOTTOM_WINGER)
				{
					if (curr->y < curr->target_y && curr->y + 1 < height)
					{
						curr->target_x = curr->x;
						curr->target_y = curr->y + 1;
					}
				}
				if (curr->role == TOP_WINGER)
				{
					if (curr->y > curr->target_y && curr->y - 1 >= 0)
					{
						curr->target_x = curr->x;
						curr->target_y = curr->y - 1;
					}
				}
				if (curr->role == ALONE_PLAYMAKER)
				{
					if (curr->x + 1 < width && game_data.map[curr->y][curr->x + 1] == 0)
					{
						curr->target_x = curr->x + 1;
						curr->target_y = curr->y;
					}
				}
			}
			if (attack_mode)
			{
				curr->target_x = attack_x_target;
				curr->target_y = attack_y_target;
			}
            if (enemy_to_splash)
			{
				printf("MOVE %d %d; THROW %d %d;\n", curr->target_x, curr->target_y, enemy_to_splash->x, enemy_to_splash->y);
				fflush(stdout);
				enemy_to_splash->curr_wetness += 30;
				if (enemy_to_splash->curr_wetness > 99)
					enemy_to_splash->died = 1;
			}
			else if (enemy_to_shoot) 
			{
				printf("MOVE %d %d; SHOOT %d;\n", curr->target_x, curr->target_y, enemy_to_shoot->agent_id);
				fflush(stdout);
				int man_dist = calculate_manhattan_distance(curr->x, enemy_to_shoot->x, curr->y, enemy_to_shoot->y);
				int damage = curr->soaking_power;
				if (man_dist > curr->optimal_range && man_dist <= curr->optimal_range * 2)
					damage /= 2;
				enemy_to_shoot->curr_wetness += damage;
				if (enemy_to_shoot->curr_wetness > 99)
					enemy_to_shoot->died = 1;
				fprintf(stderr, "enemy_wetness: %d + %d\n", enemy_to_shoot->curr_wetness, damage);
			}
			else
			{
				printf("MOVE %d %d; HUNKER_DOWN\n", curr->target_x, curr->target_y);
				fflush(stdout);
			}
            if (curr->next)
                curr = curr->next;
        }
    }
    return 0;
}