# CodinGame Summer Challenge 2025 🌞

This project showcases my participation in the **CodinGame Summer Challenge 2025** — my very first AI and algorithm competition!

## 🧠 About the Challenge

The Summer Challenge is an annual real-time programming tournament organized by [CodinGame](https://www.codingame.com/). In 2025, the proposed game challenged participants to build an autonomous bot to compete against thousands of others in a dynamic and competitive environment. Each match brought new strategies, requiring tactical adaptation, optimization, and real-time decision-making.

## 🧗 My Journey

I started coding not long ago, and this was my **first experience with AI, bots, and real-time challenges**. Every line of code was a learning moment. I entered with little knowledge but a huge desire to learn — and the progress was amazing.

## 🧠 Project Overview

The bot coordinates agents in roles such as:

- **Wingers** – Operate near the edges of the map
- **Playmakers** – Support wingers and adapt when allies die
- **Box-to-Box** – Control central map areas and seek cover
- **Alone Playmaker** – Special role when the team has few agents

Each agent selects its movement and targets based on:

- Manhattan distance
- Map terrain (cover, water)
- Wetness level (damage taken)
- Nearby enemy splash bomb threat
- Dynamic role reassignment if a teammate dies

---

## 🗺️ Map Representation

The game map is a 2D matrix where each tile is represented by:

- `0`: Empty space
- `1`: Cover (reduces 50% damage)
- `2`: High cover (reduces 75% damage)

Also added:
- `3`: Ally
- `-1`: Enemy

---

## 🧩 Core Logic

Main behavioral functions:

- `handle_playmaker_targets()` – Logic for support agents
- `handle_box_to_box_targets()` – Logic for central control agents
- `handle_wingers_targets()` – Logic for flankers
- `update_my_agent_targets()` – Master dispatcher per role

Agents react to:
- Ally deaths (and reassign roles)
- Coverage opportunities
- Wetness comparison between allies
- Splash bomb radius of enemies

---

## 🔁 Game Loop Behavior

For each game tick:
1. Read map and game state
2. Check if teammates died, reassign roles
3. Update each agent’s movement/target logic
4. Issue commands accordingly

---

🚀 **Final result: 737th place worldwide!**  
🎉 A huge achievement for me.



