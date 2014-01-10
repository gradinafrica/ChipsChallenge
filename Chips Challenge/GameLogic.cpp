//*****************************************************//
//	  Brian Gradin, 2013							   //
//	  Chip's challenge -> GameLogic.cpp				   //
//*****************************************************//
// - Contains implementations for most of the functions in the Game class
// - For descriptions of what each function does, see Game.h

#include "stdafx.h"
using namespace std;

int Game::bottomMostTile(int x, int y)
{
	return map.layers[bottomMostIndex(x, y)][x][y].get();
}

int Game::bottomMostIndex(int x, int y)
{
	// Get the id of the bottom tile at location x, y
	for (int i = map.layers.size() - 1; i >= 0; i--)
	{
		if (map.layers[i][x][y] != EMPTY_TILE)
			return i;
	}

	// No bottom; top is bottom
	return 0;
}

void Game::revealBlueWall(int x, int y)
{
	int bottomTile = bottomMostTile(x, y);

	if (bottomTile == BLUE_BLOCK_WALL_TILE || bottomTile == INVISIBLE_WALL_TILE)
		map.layers[bottomMostIndex(x, y)][x][y] = WALL_TILE;
}

bool Game::isSlippery(int x, int y, Chip chip)
{
	int bottomTile = bottomMostTile(x, y);

	return (((bottomTile == FORCE_SOUTH_TILE
		|| bottomTile == FORCE_NORTH_TILE
		|| bottomTile == FORCE_WEST_TILE
		|| bottomTile == FORCE_EAST_TILE
		|| bottomTile == FORCE_ALL_DIRECTIONS_TILE)
		&& (chip == Chip() || !chip.hasSuctionBoots))
		|| ((bottomTile == ICE_TILE
		|| bottomTile == SOUTH_EAST_ICE_TILE
		|| bottomTile == SOUTH_WEST_ICE_TILE
		|| bottomTile == NORTH_EAST_ICE_TILE
		|| bottomTile == NORTH_WEST_ICE_TILE)
		&& (chip == Chip() || !chip.hasSkates)));
}

void Game::eraseBlock(blockIterator& cur)
{
	int savedIndex = cur - movingBlocks.begin();
	movingBlocks.erase(cur);
	cur = movingBlocks.begin() + savedIndex;
}

void Game::handleMovingBlocks()
{
	int deltaX = 0, deltaY = 0;

	function<void (blockIterator&, int, int)> callMoveBlock = [&](blockIterator& cur, int newDeltaX, int newDeltaY)
	{
		cur->second.set(newDeltaX, newDeltaY);
		moveBlock(newDeltaX, newDeltaY, cur);
		DrawMap();
	};

	function<void (blockIterator&, direction&)> moveOrEraseBlockOnForceBlock = [&](blockIterator& cur, direction& newDirection)
	{
		if (!isSolid(newDirection.deltaX(), newDirection.deltaY(), cur->first.first, cur->first.second, true))
			callMoveBlock(cur, newDirection.deltaX(), newDirection.deltaY());
		else
			eraseBlock(cur);
	};
	
	function<void (blockIterator&, direction&, bool, bool)> moveOrEraseBlockOnIce = [&](blockIterator& cur, direction& newDirection, bool isCurvedIceBlock, bool alternate)
	{
		int multiplier = (isCurvedIceBlock && alternate) || !isCurvedIceBlock ? -1 : 1;
		int blockX = cur->first.first, blockY = cur->first.second;
		int deltaDeltaX = newDirection.deltaX(), deltaDeltaY = newDirection.deltaY();

		if (isSolid(deltaX = deltaDeltaX, deltaY = deltaDeltaY, blockX, blockY, true) && !isSolid(deltaDeltaY * multiplier, deltaDeltaX * multiplier, blockX, blockY, true))
		{
			deltaX = (isCurvedIceBlock ? deltaDeltaY : deltaDeltaX) * multiplier;
			deltaY = (isCurvedIceBlock ? deltaDeltaX : deltaDeltaY) * multiplier;

			if (deltaX != 0 || deltaY != 0)
				callMoveBlock(cur, deltaX, deltaY);
		}
		else if (isSolid(deltaX, deltaY, blockX, blockY, true))
			eraseBlock(cur);
		else
		{
			if (deltaX != 0 || deltaY != 0)
				callMoveBlock(cur, deltaX, deltaY);
		}
	};

	for (blockIterator cur = movingBlocks.begin(); cur != movingBlocks.end();)
	{
		switch(map.layers[bottomMostIndex(cur->first.first, cur->first.second)][cur->first.first][cur->first.second].get())
		{
		case FORCE_SOUTH_TILE:
			moveOrEraseBlockOnForceBlock(cur, direction(DOWN));
			continue;
		case FORCE_NORTH_TILE:
			moveOrEraseBlockOnForceBlock(cur, direction(UP));
			continue;
		case FORCE_EAST_TILE:
			moveOrEraseBlockOnForceBlock(cur, direction(RIGHT));
			continue;
		case FORCE_WEST_TILE:
			moveOrEraseBlockOnForceBlock(cur, direction(LEFT));
			continue;
		case FORCE_ALL_DIRECTIONS_TILE:
			moveOrEraseBlockOnForceBlock(cur, direction(rand() % 4));
			continue;
		case TRAP_TILE:
		case ICE_TILE:
			if (cur->second == UP)
			{
				moveOrEraseBlockOnIce(cur, direction(UP), false, false);
				continue;
			}

			if (cur->second == DOWN)
			{
				moveOrEraseBlockOnIce(cur, direction(DOWN), false, false);
				continue;
			}

			if (cur->second == LEFT)
			{
				moveOrEraseBlockOnIce(cur, direction(LEFT), false, false);
				continue;
			}

			if (cur->second == RIGHT)
			{
				moveOrEraseBlockOnIce(cur, direction(RIGHT), false, false);
				continue;
			}

			continue;
		case SOUTH_EAST_ICE_TILE:
			if (cur->second == UP)
				moveOrEraseBlockOnIce(cur, direction(RIGHT), true, false);
			else
				moveOrEraseBlockOnIce(cur, direction(DOWN), true, false);

			continue;
		case SOUTH_WEST_ICE_TILE:
			if (cur->second == UP)
				moveOrEraseBlockOnIce(cur, direction(LEFT), true, true);
			else
				moveOrEraseBlockOnIce(cur, direction(DOWN), true, true);
			
			continue;
		case NORTH_WEST_ICE_TILE:
			if (cur->second == DOWN)
				moveOrEraseBlockOnIce(cur, direction(LEFT), true, false);
			else
				moveOrEraseBlockOnIce(cur, direction(UP), true, false);

			continue;
		case NORTH_EAST_ICE_TILE:
			if (cur->second == DOWN)
				moveOrEraseBlockOnIce(cur, direction(RIGHT), true, true);
			else
				moveOrEraseBlockOnIce(cur, direction(UP), true, true);

			continue;
		}
	}

	if (map.layers[0][chip.x][chip.y] == MUD_BLOCK_TILE)
		death(5);
}

void Game::redraw(int tile, int x, int y, int deltaX, int deltaY)
{
	redrawOldTile(tile, x, y);
	redrawNewTile(tile, x + deltaX, y + deltaY);
}

void Game::redrawOldTile(int tile, int x, int y)
{
	int layer = 0;

	// Find what layer the moving tile is on
	for (unsigned int i = 0; i < map.layers.size(); i++)
	{
		if (map.layers[i][x][y] == tile || (tile > 63 && (int)(tile / 4) == (int)(map.layers[i][x][y].get() / 4)))
		{
			layer = i;
			break;
		}
	}

	// Redraw the tile(s) underneath the moving tile on the position the moving tile is leaving
	for (unsigned int i = layer; i < map.layers.size(); i++)
	{
		if ((i + 1) < map.layers.size() && map.layers[i + 1][x][y] != 0)
			map.layers[i][x][y] = map.layers[i + 1][x][y].get();
		else
			map.layers[i][x][y] = EMPTY_TILE;
	}

	if (map.layers[map.layers.size() - 1].isEmpty() && map.layers.size() > 2)
		// The bottom layer is empty.  Delete it
		map.layers.pop_back();
}

void Game::redrawNewTile(int tile, int x, int y)
{
	int holder = map.layers[0][x][y].get();

	// Redraw all tiles on the position to which the moving tile is moving
	for (unsigned int i = 1; i < map.layers.size(); i++)
	{
		int tmp = holder;
		holder = map.layers[i][x][y].get();
		map.layers[i][x][y] = tmp;
	}

	if (holder != 0)
	{
		// If there was something in the bottom layer, add a new layer and put the remainder in it
		map.layers.push_back(MapLayer());
		map.layers[map.layers.size() - 1][x][y] = holder;
	}
	else if (map.layers[map.layers.size() - 1].isEmpty() && map.layers.size() > 2)
	{
		// The bottom layer is empty.  Delete it
		map.layers.pop_back();
	}

	// Place the actual block (by convention, it goes on top)
	map.layers[0][x][y] = tile;
}

void Game::moveBlock(int deltaX, int deltaY, blockIterator& cur)
{
	int x = cur->first.first;
	int y = cur->first.second;

	// Common movement of all moveable objects
	commonMovement(x, y, deltaX, deltaY, deltaX, deltaY, 0, true);

	// Redraw necessary tiles
	redraw(MUD_BLOCK_TILE, x, y, deltaX, deltaY);

	int savedIndex = cur - movingBlocks.begin(); // save index

	// If the block landed on a clone block, make sure we still have a valid iterator
	if (handleClonerButton(x, y, deltaX, deltaY))
		cur = movingBlocks.begin() + savedIndex + 1;

	bool blockToErase = false;
	int bottom = bottomMostTile(x + deltaX, y + deltaY);

	if (bottom == WATER_TILE)
	{
		soundEffects["SplashSound"].play();

		// Redraw necessary tiles
		redrawOldTile(MUD_BLOCK_TILE, x + deltaX, y + deltaY);

		map.layers[bottomMostIndex(x + deltaX, y + deltaY)][x + deltaX][y + deltaY] = DIRT_TILE;

		// We're not going to add back in this block, because it turned into dirt
		blockToErase = true;
	}

	if (bottom == BOMB_TILE)
	{
		redrawOldTile(MUD_BLOCK_TILE, x + deltaX, y + deltaY);
		soundEffects["BombSound"].play();
		map.layers[bottomMostIndex(x + deltaX, y + deltaY)][x + deltaX][y + deltaY] = 0;
		blockToErase = true;
	}

	// if the block is no longer on something slippery
	if (!isSlippery(x + deltaX, y + deltaY) && bottomMostTile(x + deltaX, y + deltaY) != TRAP_TILE) 
		blockToErase = true;

	if (blockToErase)
		eraseBlock(cur);
	else
	{
		cur->first.first += deltaX;
		cur->first.second += deltaY;
		cur++; // Keep the loop to handle blocks moving forward
	}
}

void Game::handleMonsters()
{
	if (!monstersCanMove)
	{
		monstersCanMove = true;
		return;
	}

	if (!monsters.empty())
	{
		int deltaX, deltaY;

		// Use starting index of the end to keep us from moving monsters we create
		int index = monsters.end() - monsters.begin();

		function<void (deque<Monster>::iterator&, direction&)> handleForceBlock = [&](deque<Monster>::iterator& cur, direction& forceDirection)
		{
			if (!isSolid(deltaX = forceDirection.deltaX(), deltaY = forceDirection.deltaY(), cur->x, cur->y, cur->type))
				cur->currentDirection = forceDirection;
			else
				deltaX = deltaY = 0;
		};

		function<void (deque<Monster>::iterator&, direction&, bool, bool)> handleIceBlock = [&](deque<Monster>::iterator& cur, direction& newDirection, bool isCurvedIceBlock, bool alternate)
		{
			int multiplier = !isCurvedIceBlock || alternate ? -1 : 1;
			int multiplierX = (isCurvedIceBlock ? newDirection.deltaY() : newDirection.deltaX()) * multiplier;
			int multiplierY = (isCurvedIceBlock ? newDirection.deltaX() : newDirection.deltaY()) * multiplier;

			if (isSolid(deltaX = newDirection.deltaX(), deltaY = newDirection.deltaY(), cur->x, cur->y, cur->type) && !isSolid(multiplierX, multiplierY, cur->x, cur->y, cur->type))
			{
				deltaX = multiplierX;
				deltaY = multiplierY;
			}
			else if (isSolid(deltaX, deltaY, cur->x, cur->y, cur->type))
				deltaX = deltaY = 0;
			
			if (deltaX != 0 || deltaY != 0)
				cur->currentDirection.set(deltaX, deltaY);
		};

		// Iterate through monster list
		for (deque<Monster>::iterator cur = monsters.begin(); isActive && cur != monsters.begin() + index && cur != monsters.end(); (monsters.size() != 0 && cur != monsters.end()) ? cur++ : cur)
		{
			bool found = false;
			deltaX = deltaY = 0;

			int bottomTile = bottomMostTile(cur->x, cur->y);

			// If monster is not on an ice or teleport block, only allow it to move selectively.
			if (!isSlippery(cur->x, cur->y))
			{
				// Only allow the monster to move selectively
				if (cur->movedRecently || cur->skipFrame)
				{
					if (!cur->movedRecently)
					{
						cur->skipFrame = false;
						cur->movedRecently = true;
					}
					else
						cur->movedRecently = false;
					continue;
				}
				else
				{
					if (cur->type == TEETH || cur->type == BLOB)
						cur->skipFrame = true;

					cur->movedRecently = true;
				}
			}
			else
			{
				switch(bottomTile)
				{
				case FORCE_SOUTH_TILE:
					handleForceBlock(cur, direction(DOWN));
					break;
				case FORCE_NORTH_TILE:
					handleForceBlock(cur, direction(UP));
					break;
				case FORCE_EAST_TILE:
					handleForceBlock(cur, direction(RIGHT));
					break;
				case FORCE_WEST_TILE:
					handleForceBlock(cur, direction(LEFT));
					break;
				case FORCE_ALL_DIRECTIONS_TILE:
					{
						direction newDirection(rand() % 4);
						if (!isSolid(newDirection.deltaX(), newDirection.deltaY(), cur->x, cur->y, cur->type))
							handleForceBlock(cur, newDirection);
						else
							continue;
					}
					break;
				case ICE_TILE:
					handleIceBlock(cur, cur->currentDirection, false, false);
					break;
				case SOUTH_EAST_ICE_TILE:
					if (cur->currentDirection == UP)
						handleIceBlock(cur, direction(RIGHT), true, false);
					else
						handleIceBlock(cur, direction(DOWN), true, false);
					break;
				case SOUTH_WEST_ICE_TILE:
					if (cur->currentDirection == UP)
						handleIceBlock(cur, direction(LEFT), true, true);
					else
						handleIceBlock(cur, direction(DOWN), true, true);
					break;
				case NORTH_WEST_ICE_TILE:
					if (cur->currentDirection == DOWN)
						handleIceBlock(cur, direction(LEFT), true, false);
					else
						handleIceBlock(cur, direction(UP), true, false);
					break;
				case NORTH_EAST_ICE_TILE:
					if (cur->currentDirection == DOWN)
						handleIceBlock(cur, direction(RIGHT), true, true);
					else
						handleIceBlock(cur, direction(UP), true, true);
					break;
				}

				if (deltaX != 0 || deltaY != 0)
				{
					moveMonster(deltaX, deltaY, cur, index);
					DrawMap();
				}
				continue;
			}

			// Bug and centipede
			if (cur->type == BUG || cur->type == PARAMECIUM)
			{
				for (int i = 0; i < 4 && !found; i++)
				{
					// Calculate x and y offset
					int newDirectionCode = cur->currentDirection.toInt() + 1;
					direction newDirection = direction(newDirectionCode > 3 ? 0 : newDirectionCode);
					deltaX = newDirection.deltaX();
					deltaY = newDirection.deltaY();

					if (cur->type == PARAMECIUM)
					{
						deltaX *= -1;
						deltaY *= -1;
					}

					// Bug
					// If it can turn left, do so.  Otherwise, turn right until there's an opening.
					if (cur->type == BUG && isSolid(deltaX, deltaY, cur->x, cur->y, cur->type))
					{
						if (bottomTile != CLONING_MACHINE_TILE)
						{
							if (cur->currentDirection == UP)
								cur->currentDirection = RIGHT;
							else
								cur->currentDirection = cur->currentDirection.toInt() - 1;
						}
					}
					else if (cur->type == BUG)
					{
						if (bottomTile != CLONING_MACHINE_TILE)
						{
							if (cur->currentDirection == RIGHT)
								cur->currentDirection = UP;
							else
								cur->currentDirection = cur->currentDirection.toInt() + 1;
						}
						found = true;
					}
					else if (cur->type == PARAMECIUM && isSolid(deltaX, deltaY, cur->x, cur->y, cur->type))
					{	
						if (bottomTile != CLONING_MACHINE_TILE)
						{
							if (cur->currentDirection == RIGHT)
								cur->currentDirection = UP;
							else
								cur->currentDirection = cur->currentDirection.toInt() + 1;
						}
					}
					else
					{
						if (bottomTile != CLONING_MACHINE_TILE)
						{
							if (cur->currentDirection == UP)
								cur->currentDirection = RIGHT;
							else
								cur->currentDirection = cur->currentDirection.toInt() - 1;
						}
						found = true;
					}
				}
			}

			// Fire bug, Pink ball, Tank, Dumbbell and Ghost
			if ((cur->type > 16 && cur->type < 21) || cur->type == WALKER)
			{
				bool canContinue = false;

				// If it can keep going straight, do so.  Otherwise, turn.
				for (int i = 0; !found && ((i < 2 && cur->type == PINK_BALL) || (i < 1)) && !canContinue; i++)
				{
					// Get x and y offset
					deltaX = cur->currentDirection.deltaX();
					deltaY = cur->currentDirection.deltaY();

					// Concussion rule
					if (bottomTile == TRAP_TILE && isSolid(deltaX, deltaY, cur->x, cur->y, cur->type) && cur->type != TANK)
					{
						canContinue = true;
						continue;
					}

					if (isSolid(deltaX, deltaY, cur->x, cur->y, cur->type))
					{
						if (cur->type == TANK)
							cur->canMove = false;
						else if (cur->type == FIRE_BUG)
						{
							swap(deltaX, deltaY);
							deltaX *= -1;

							if (isSolid(deltaX, deltaY, cur->x, cur->y, cur->type))
							{
								deltaX *= -1;
								deltaY *= -1;

								if (isSolid(deltaX, deltaY, cur->x, cur->y, cur->type))
								{
									swap(deltaX, deltaY);
									deltaY *= -1;

									if (!isSolid(deltaX, deltaY, cur->x, cur->y, cur->type))
										found = true;
								}
								else
									found = true;
							}
							else
								found = true;

							if (bottomTile != CLONING_MACHINE_TILE)
								cur->currentDirection.set(deltaX, deltaY);
						}
						else if (cur->type == PINK_BALL)
						{
							if (bottomTile != CLONING_MACHINE_TILE)
								cur->currentDirection.set(deltaX * -1, deltaY * -1);
						}
						else if (cur->type == WALKER)
						{
							if (bottomTile != CLONING_MACHINE_TILE)
								cur->currentDirection = rand() % 4;
							
							deltaX = cur->currentDirection.deltaX();
							deltaY = cur->currentDirection.deltaY();
							
							if (!isSolid(deltaX, deltaY, cur->x, cur->y, cur->type))
								found = true;
						}
						else // Ghost
						{
							swap(deltaX, deltaY);
							deltaY *= -1;

							if (isSolid(deltaX, deltaY, cur->x, cur->y, cur->type))
							{
								deltaX *= -1;
								deltaY *= -1;

								if (isSolid(deltaX, deltaY, cur->x, cur->y, cur->type))
								{
									swap(deltaX, deltaY);
									deltaX *= -1;

									if (!isSolid(deltaX, deltaY, cur->x, cur->y, cur->type))
										found = true;
								}
								else
									found = true;
							}
							else
								found = true;

							if (bottomTile != CLONING_MACHINE_TILE)
								cur->currentDirection.set(deltaX, deltaY);
						}
					}
					else
						found = true;
				}

				if (canContinue)
					continue;
			}

			// Frog
			if (cur->type == TEETH)
			{
				// Set up distance measurement variables
				int tempDeltaX = chip.x - cur->x;
				int tempDeltaY = chip.y - cur->y;
				deltaX   = (tempDeltaX == 0) ? 0 : tempDeltaX / abs(tempDeltaX);
				deltaY   = (tempDeltaY == 0) ? 0 : tempDeltaY / abs(tempDeltaY);
				bool x_clear = !isSolid(deltaX, 0, cur->x, cur->y, cur->type);
				bool y_clear = !isSolid(0, deltaY, cur->x, cur->y, cur->type);

				direction tmp = cur->currentDirection;

				// Get as close to chip as possible
				if ((abs(tempDeltaX) > abs(tempDeltaY) && x_clear) || (!y_clear && x_clear))
					found = (deltaY = 0) != 1;
				else if (y_clear)
					found = (deltaX = 0) != 1;

				// Change monster orientation
				if (bottomTile != CLONING_MACHINE_TILE)
				{
					if (!found && abs(tempDeltaX) > abs(tempDeltaY))
						deltaY = 0;
					else if (!found)
						deltaX = 0;

					cur->currentDirection.set(deltaX, deltaY);
				}

				if (!found && tmp != cur->currentDirection)
					moveMonster(0, 0, cur, index);
			}

			// Blob
			if (cur->type == BLOB)
			{
				direction newDirection(rand() % 4);
				found = !isSolid(deltaX = newDirection.deltaX(), deltaY = newDirection.deltaY(), cur->x, cur->y, cur->type);
			}

			if (found)
				moveMonster(deltaX, deltaY, cur, index);
		}

		DrawMap();
	}
}

void Game::moveMonster(int deltaX, int deltaY, deque<Monster>::iterator& cur, int& lastPosition)
{
	// Common movement of all moveable objects
	commonMovement(cur->x, cur->y, deltaX, deltaY, deltaX, deltaY, true);

	// Redraw the necessary tiles
	redraw((cur->type * 4) + cur->currentDirection.toInt(), cur->x, cur->y, deltaX, deltaY);

	int index = cur - monsters.begin();

	// If the monster landed on a clone block, make sure we still have a valid iterator
	if (handleClonerButton(cur->x, cur->y, deltaX, deltaY))
		cur = monsters.begin() + index;

	// Set up new coordinates
	cur->x += deltaX;
	cur->y += deltaY;

	// If the monster steps in water, fire or on a bomb
	// Ghosts cannot be killed by water
	int bottom_tile = bottomMostTile(cur->x, cur->y);
	if ((bottom_tile == WATER_TILE && cur->type != GLIDER)
		|| (bottom_tile == FIRE_TILE && cur->type != FIRE_BUG)
		|| bottom_tile == BOMB_TILE)
	{
		if (bottom_tile == BOMB_TILE)
			soundEffects["BombSound"].play();

		// Redraw tile
		redrawOldTile((cur->type * 4) + cur->currentDirection.toInt(), cur->x, cur->y);
		if (bottom_tile == BOMB_TILE)
		{
			int bottom = bottomMostIndex(cur->x, cur->y);

			// Redraw bottom tile
			map.layers[bottom][cur->x][cur->y] = EMPTY_TILE;

			// Get rid of the bottom layer if it's empty
			if (map.layers[bottom].isEmpty() && map.layers.size() > 2)
				map.layers.pop_back();
		}

		int index = cur - monsters.begin();

		monsters.erase(cur);

		cur = monsters.begin() + index;

		lastPosition--;

		if (cur != monsters.begin())
			cur--;
	}
}

void Game::incrementTriesAndReloadMap()
{
	totalTries++;

	if (timeLeft < map.timelimit - 10)
		actualTries++;

	if (actualTries == 11)
	{
		if (MessageBox(hWnd, L"You seem to be having trouble with this level. Would you like to skip to the next level?", L"Chip's Challenge", MB_YESNO) == IDYES)
			map.Load(*this, map.levelnumber + 1);
		else
		{
			actualTries = 0;
			map.Load(*this, map.levelnumber);
		}
	}
	else
		map.Load(*this, map.levelnumber);
}

void Game::death(int code)
{
	soundEffects["ChipDeathSound"].play();

	chip.isDead = true;
	isActive = false;

	string msg;

	switch (code)
	{
	case 1:
		msg = "Oops! Chip can't swim without hasFlippers!";
		break;
	case 2:
		msg = "Oops! Don't step in the fire without fire boots!";
		break;
	case 3:
		msg = "Ooops! Look out for creatures!";
		break;
	case 4:
		msg = "Oops! Don't touch the bombs!";
		break;
	case 5:
		msg = "Ooops! Watch out for moving blocks!";
		break;
	}
	
	DrawMap();
	MessageBox(hWnd, stows(msg).c_str(), L"Chip's Challenge", MB_OK);

	incrementTriesAndReloadMap();
}

void Game::moveChip(int deltaX, int deltaY)
{
	// Conditions where Chip shouldn't move
	if (isSolid(deltaX, deltaY, chip.x, chip.y))
	{
		if (deltaX != 0 || deltaY != 0)
			soundEffects["BlockedMoveSound"].play();

		// In case he hit a wall which should be revealed
		revealBlueWall(chip.x + deltaX, chip.y + deltaY);

		deltaX = deltaY = 0;
	}

	// Store deltaX and deltaY for later, in case we need them to calculate block movement
	int storedDeltaX = deltaX;
	int storedDeltaY = deltaY;

	// Handle common movement elements of all moveable objects
	if (deltaX != 0 || deltaY != 0)
		commonMovement(chip.x, chip.y, deltaX, deltaY, deltaX, deltaY);

	// Declare some variables which will come in handy later in this function
	int newX = chip.x + deltaX, newY = chip.y + deltaY;
	int newID = map.layers[0][newX][newY].get();
	int bottomIndex, blockToErase = -1;
	bool select = false;

	if (deltaX != 0 || deltaY != 0)
	{
		// Conditions where chip lands on something or gets an item
		for (unsigned int i = 0; i < map.layers.size(); i++)
		{
			switch (blockToErase = map.layers[i][newX][newY].get())
			{
			case CHIP_TILE:
				if(chipsLeft > 0)
					chipsLeft--;
			
				soundEffects["PickUpChipSound"].play();
				break;
			case BLUE_DOOR_TILE:
				if (chip.blueKeys == 0)
					break;
			
				chip.blueKeys--;
			
				soundEffects["OpenDoorSound"].play();
				break;
			case RED_DOOR_TILE:
				if (chip.redKeys == 0)
					break;

				chip.redKeys--;
			
				soundEffects["OpenDoorSound"].play();
				break;
			case GREEN_DOOR_TILE:
				if (!chip.hasGreenKey)
					break;
			
				soundEffects["OpenDoorSound"].play();
				break;
			case YELLOW_DOOR_TILE:
				if (chip.yellowKeys == 0)
					break;
			
				chip.yellowKeys--;
			
				soundEffects["OpenDoorSound"].play();
				break;
			case BLUE_KEY_TILE:
				chip.blueKeys++;
				soundEffects["PickUpToolSound"].play();
				break;
			case RED_KEY_TILE:
				chip.redKeys++;
				soundEffects["PickUpToolSound"].play();
				break;
			case GREEN_KEY_TILE:
				chip.hasGreenKey = true;
				soundEffects["PickUpToolSound"].play();
				break;
			case YELLOW_KEY_TILE:
				chip.yellowKeys++;
				soundEffects["PickUpToolSound"].play();
				break;
			case FLIPPERS_TILE:
				chip.hasFlippers = true;
				soundEffects["PickUpToolSound"].play();
				break;
			case FIRE_BOOTS_TILE:
				chip.hasFireBoots = true;
				soundEffects["PickUpToolSound"].play();
				break;
			case SKATES_TILE:
				chip.hasSkates = true;
				soundEffects["PickUpToolSound"].play();
				break;
			case SUCTION_BOOTS_TILE:
				chip.hasSuctionBoots = true;
				soundEffects["PickUpToolSound"].play();
				break;
			case SOCKET_TILE:
				if (chipsLeft != 0)
					break;

				soundEffects["SocketSound"].play(); // Socket
				break;
			case DIRT_TILE:
			case BLUE_BLOCK_TILE_TILE:
				break;
			default:
				blockToErase = -1;
			}

			if (blockToErase != -1)
				break;
		}

		if (blockToErase != -1)
			redrawOldTile(blockToErase, newX, newY);
		blockToErase = -1;

		bottomIndex = bottomMostIndex(newX, newY);

		if (map.layers[bottomIndex][newX][newY] == THIEF_TILE)
		{
			chip.hasSuctionBoots = chip.hasFlippers = chip.hasSkates = chip.hasFireBoots = false;
			soundEffects["ThiefSound"].play();
		}

		if (map.layers[bottomIndex][newX][newY] == PASS_ONCE_TILE)
			map.layers[0][newX][newY] = WALL_TILE;

		if (map.layers[bottomIndex][newX][newY] == HINT_TILE)
			chip.isOnHintTile = true;
		else if (chip.isOnHintTile)
			chip.isOnHintTile = false;

		bool movedblock = false;

		if (newID == MUD_BLOCK_TILE)
		{
			int newBottomIndex = bottomMostIndex(chip.x + deltaX + storedDeltaX, chip.y + deltaY + storedDeltaY);

			// First things first - find out if the block is in the moving blocks list
			for (deque<pair<pair<int, int>, direction>>::iterator cur = movingBlocks.begin(); cur != movingBlocks.end(); cur++)
			{
				if (chip.x + deltaX == cur->first.first && chip.y + deltaY == cur->first.second)
				{
					movingBlocks.erase(cur);
					break;
				}
			}
		
			if (map.layers[newBottomIndex][chip.x + deltaX + storedDeltaX][chip.y + deltaY + storedDeltaY] != WATER_TILE)
			{
				if (map.layers[newBottomIndex][chip.x + deltaX + storedDeltaX][chip.y + deltaY + storedDeltaY] == BOMB_TILE)
				{
					redrawOldTile(MUD_BLOCK_TILE, chip.x + deltaX, chip.y + deltaY);
					soundEffects["BombSound"].play();
					map.layers[newBottomIndex][chip.x + deltaX + storedDeltaX][chip.y + deltaY + storedDeltaY] = EMPTY_TILE;
				}
				else
				{
					// Common movement of all moveable objects determines if the block moves or not
					if (commonMovement(chip.x + deltaX, chip.y + deltaY, deltaX, deltaY, storedDeltaX, storedDeltaY))
					{
						// Redraw necessary tiles
						redrawNewTile(MUD_BLOCK_TILE, chip.x + deltaX + storedDeltaX, chip.y + deltaY + storedDeltaY);

						movedblock = true;

						if (isSlippery(chip.x + deltaX + storedDeltaX, chip.y + deltaY + storedDeltaY))
							movingBlocks.push_front(make_pair(make_pair(chip.x + deltaX + storedDeltaX, chip.y + deltaY + storedDeltaY), direction().set(deltaX, deltaY)));

						handleClonerButton(chip.x + deltaX, chip.x + deltaY, storedDeltaX, storedDeltaY);
					}
					else if (storedDeltaX == 0 && storedDeltaY == 0)
					{
						soundEffects["BlockedMoveSound"].play();
						deltaX = deltaY = 0;
					}
				}
			}
			else
			{
				soundEffects["SplashSound"].play();
				int newBottomIndex = bottomMostIndex(chip.x + deltaX + storedDeltaX, chip.y + deltaY + storedDeltaY);
				map.layers[newBottomIndex][chip.x + deltaX + storedDeltaX][chip.y + deltaY + storedDeltaY] = DIRT_TILE;
			}

			blockToErase = MUD_BLOCK_TILE;
		}

		if (blockToErase != -1)
			redrawOldTile(blockToErase, newX, newY);

		// Check to see if he landed on a cloner
		handleClonerButton(chip.x, chip.y, deltaX, deltaY);

		// Check to see if a block hit a cloner
		if (movedblock)
			handleClonerButton(chip.x + deltaX, chip.y + deltaY, storedDeltaX, storedDeltaY);
	}

	bottomIndex = bottomMostIndex(newX, newY);

	// Redraw the necessary tiles.  Put 0 at the top for now
	if (map.layers[bottomIndex][newX][newY] != WATER_TILE)
		redraw(CHIP_NORTH_TILE + chip.currentDirection.toInt(), chip.x, chip.y, deltaX, deltaY);
	else
		redraw(CHIP_SWIMMING_NORTH_TILE + chip.currentDirection.toInt(), chip.x, chip.y, deltaX, deltaY);

	bottomIndex = bottomMostIndex(newX, newY);

	// Draw Chip
	if (map.layers[bottomIndex][newX][newY] != WATER_TILE)
	{
		if (chip.notForward)
		{
			if (chip.currentDirection == UP)
				map.layers[0][newX][newY] = CHIP_NORTH_TILE;
			else if (chip.currentDirection == RIGHT)
				map.layers[0][newX][newY] = CHIP_EAST_TILE;
			else if (chip.currentDirection == DOWN)
				map.layers[0][newX][newY] = CHIP_SOUTH_TILE;
			else if (chip.currentDirection == LEFT)
				map.layers[0][newX][newY] = CHIP_WEST_TILE;
		}
		else
			map.layers[0][newX][newY] = CHIP_SOUTH_TILE;
	}
	else
	{
		if (chip.notForward)
		{
			if (chip.currentDirection == UP)
				map.layers[0][newX][newY] = CHIP_SWIMMING_NORTH_TILE;
			else if (chip.currentDirection == RIGHT)
				map.layers[0][newX][newY] = CHIP_SWIMMING_EAST_TILE;
			else if (chip.currentDirection == DOWN)
				map.layers[0][newX][newY] = CHIP_SWIMMING_SOUTH_TILE;
			else if (chip.currentDirection == LEFT)
				map.layers[0][newX][newY] = CHIP_SWIMMING_WEST_TILE;
		}
		else
			map.layers[0][newX][newY] = CHIP_SWIMMING_SOUTH_TILE;
	}

	// Set up movement attributes
	chip.lastMove = (FRAMES_PER_SECOND / 2);

	// Set up new coordinates
	chip.x = newX;
	chip.y = newY;

	if (map.layers[bottomIndex][newX][newY] == WATER_TILE)
	{
		if (!chip.hasFlippers)
		{
			map.layers[0][newX][newY] = DROWNING_CHIP_TILE;
			return death(1);
		}
	}
	if (map.layers[bottomIndex][newX][newY] == FIRE_TILE && !chip.hasFireBoots)
	{
		map.layers[0][newX][newY] = BURNED_CHIP_TILE1;
		return death(2);
	}
	if (map.layers[bottomIndex][newX][newY] == BOMB_TILE)
	{
		map.layers[0][newX][newY] = BURNED_CHIP_TILE1;
		return death(4);
	}

	if (chip.x == clickedPoint.x && chip.y == clickedPoint.y)
		clickedPoint.x = clickedPoint.y = 50;

	if (chipHasHitMonster())
	{
		int monsterType = -1;
		for (unsigned int i = 0; i < map.layers.size(); i++)
			if (map.layers[i][chip.x][chip.y] > 63 && map.layers[i][chip.x][chip.y] < 100)
				monsterType = map.layers[i][chip.x][chip.y].get();

		if (monsterType != -1 && map.layers[0][chip.x][chip.y].get() % 4 == (monsterType + 2) % 4)
			return death(3);
	}

	if (map.layers[bottomIndex][chip.x][chip.y] == EXIT_TILE)
	{
		soundEffects["LevelCompleteSound"].play();

		chip.isDead = true;

		isActive = false;
		DrawMap();
		CallVictory();

		if (map.levelnumber == 50)
			MessageBox(hWnd, L"Picking up chips is what the challenge is all about. But on the ice, Chip gets chapped and feels like a chump instead of a champ.", L"Chip's Challenge", MB_OK);
		if (map.levelnumber == 60)
			MessageBox(hWnd, L"Chip hits the ice and decides to chill out. Then he runs into a fake wall and turns the maze into a thrash-a-thon!", L"Chip's Challenge", MB_OK);
		if (map.levelnumber == 70)
			MessageBox(hWnd, L"Chip is halfway through the world's hardest puzzle. If he succeeds, maybe the kids will stop calling him computer breath!", L"Chip's Challenge", MB_OK);
		if (map.levelnumber == 80)
			MessageBox(hWnd, L"Chip used to spend his time programming computer games and making models. But that was just practice for this brain-buster!", L"Chip's Challenge", MB_OK);
		if (map.levelnumber == 90)
			MessageBox(hWnd, L"\"I can do it! I know I can!\" Chip thinks as the going gets tougher. Besides, Melinda the Mental Marvel waits at the end!", L"Chip's Challenge", MB_OK);
		if (map.levelnumber == 100)
			MessageBox(hWnd, L"Besides being an angel on earth, Melinda is the top scorer in the Challenge�and the president of the Bit Busters.", L"Chip's Challenge", MB_OK);
		if (map.levelnumber == 110)
			MessageBox(hWnd, L"Chip can't wait to join the Bit Busters! The club's already figured out the school's password and accessed everyone's grades!", L"Chip's Challenge", MB_OK);
		if (map.levelnumber == 120)
			MessageBox(hWnd, L"If Chip's grades aren't as good as Melinda's, maybe she'll come over to his house and help him study!", L"Chip's Challenge", MB_OK);
		if (map.levelnumber == 130)
			MessageBox(hWnd, L"I've made it this far,' Chip thinks. 'Totally fair, with my mega-brain.' Then he starts the next maze. 'Totally unfair!' he yelps.", L"Chip's Challenge", MB_OK);
		if (map.levelnumber == 140)
			MessageBox(hWnd, L"Groov-u-loids! Chip makes it almost to the end. He's stoked!", L"Chip's Challenge", MB_OK);

		if (map.levelnumber + 1 <= totalLevels) map.Load(*this, map.levelnumber + 1);
		else if (map.levelnumber == 144 || map.levelnumber == totalLevels)
			isBeaten = true;
		return;
	}
}

bool Game::chipHasHitMonster()
{
	for (unsigned int i = 0; i < map.layers.size(); i++)
	{
		int newID = map.layers[i][chip.x][chip.y].get();
		if (newID >= BUG_NORTH_TILE && newID <= PARAMECIUM_EAST_TILE)
			return true;
	}
	return false;
}

bool Game::isSolid(int deltaX, int deltaY, int x, int y, int t) // Used for monsters
{
	int bottomTile = bottomMostTile(x, y);
	int newBottomTile = bottomMostTile(x + deltaX, y + deltaY);

	if (x < 0 || y < 0 || x > 31 || y > 31 // Out of bounds
	 || newBottomTile == WALL_TILE
	 || newBottomTile == CHIP_TILE
	 || newBottomTile == INVISIBLE_WALL_NO_APPEARANCE_TILE
	 || newBottomTile == DIRT_TILE
	 || newBottomTile == EXIT_TILE
	 || newBottomTile == BLUE_DOOR_TILE
	 || newBottomTile == RED_DOOR_TILE
	 || newBottomTile == GREEN_DOOR_TILE
	 || newBottomTile == YELLOW_DOOR_TILE
	 || newBottomTile == BLUE_BLOCK_TILE_TILE
	 || newBottomTile == BLUE_BLOCK_WALL_TILE
	 || newBottomTile == THIEF_TILE
	 || newBottomTile == SOCKET_TILE
	 || newBottomTile == SWITCH_BLOCK_CLOSED_TILE
	 || newBottomTile == INVISIBLE_WALL_TILE
	 || newBottomTile == GRAVEL_TILE
	 || newBottomTile == PASS_ONCE_TILE
	 || newBottomTile == CLONING_MACHINE_TILE
	 || (((deltaX < 0 || deltaY < 0) && bottomTile == SOUTH_EAST_ICE_TILE) || ((deltaX > 0 || deltaY > 0) && newBottomTile == SOUTH_EAST_ICE_TILE))
	 || (((deltaX > 0 || deltaY < 0) && bottomTile == SOUTH_WEST_ICE_TILE) || ((deltaX < 0 || deltaY > 0) && newBottomTile == SOUTH_WEST_ICE_TILE))
	 || (((deltaX > 0 || deltaY > 0) && bottomTile == NORTH_WEST_ICE_TILE) || ((deltaX < 0 || deltaY < 0) && newBottomTile == NORTH_WEST_ICE_TILE))
	 || (((deltaX < 0 || deltaY > 0) && bottomTile == NORTH_EAST_ICE_TILE) || ((deltaX > 0 || deltaY < 0) && newBottomTile == NORTH_EAST_ICE_TILE))
	 || ((deltaY  >  0  && newBottomTile == BLOCKED_NORTH_TILE) || (deltaY < 0 && bottomTile == BLOCKED_NORTH_TILE))
     || ((deltaX  >  0  && newBottomTile == BLOCKED_WEST_TILE) || (deltaX < 0 && bottomTile == BLOCKED_WEST_TILE))
     || ((deltaY  <  0  && newBottomTile == BLOCKED_SOUTH_TILE) || (deltaY > 0 && bottomTile == BLOCKED_SOUTH_TILE))
     || ((deltaX  <  0  && newBottomTile == BLOCKED_EAST_TILE) || (deltaX > 0 && bottomTile == BLOCKED_EAST_TILE))
     || (((deltaY < 0 || deltaX < 0) && newBottomTile == BLOCKED_SOUTH_EAST_TILE) 
	 || ((deltaY > 0 || deltaX > 0) && bottomTile == BLOCKED_SOUTH_EAST_TILE)))
		return true;

	for (unsigned int i = 0; i < map.layers.size(); i++)
	{
		int newtile = map.layers[i][x + deltaX][y + deltaY].get();

		if (newtile == MUD_BLOCK_TILE
		|| (newtile == FIRE_TILE && (t == BUG || t == WALKER))
		|| (newtile >= FLIPPERS_TILE && newtile <= SUCTION_BOOTS_TILE)
	    || (newtile >= BUG_NORTH_TILE && newtile < PARAMECIUM_EAST_TILE)) // Another monster
			return true;
	}

	return false;
}

bool Game::isSolid(int deltaX, int deltaY, int oldX, int oldY, bool isBlock) // Used for Chip and blocks
{
	int x = oldX + deltaX;
	int y = oldY + deltaY;

	int bottomTile = bottomMostTile(oldX, oldY);
	int newBottomTile = bottomMostTile(x, y);

	if (x < 0 || y < 0 || x > 31 || y > 31
	 || newBottomTile == WALL_TILE
	 || (isBlock && (newBottomTile == BLUE_BLOCK_TILE_TILE
	 || newBottomTile == THIEF_TILE
	 || newBottomTile == DIRT_TILE
	 || newBottomTile == CHIP_TILE
	 || (map.layers[0][x][y] >= BLUE_DOOR_TILE && map.layers[0][x][y] <= YELLOW_DOOR_TILE)
	 || newBottomTile == SOCKET_TILE))

	 || newBottomTile == BLUE_BLOCK_WALL_TILE
	 || newBottomTile == SWITCH_BLOCK_CLOSED_TILE
	 || (newBottomTile == TRAP_TILE && map.layers[0][x][y] == MUD_BLOCK_TILE)
	 || newBottomTile == INVISIBLE_WALL_NO_APPEARANCE_TILE
	 || newBottomTile == INVISIBLE_WALL_TILE
	 || newBottomTile == CLONING_MACHINE_TILE

	 || (isBlock && ((newBottomTile >= SOUTH_EAST_ICE_TILE && newBottomTile <= NORTH_EAST_ICE_TILE || newBottomTile == ICE_TILE) && map.layers[0][x][y] == MUD_BLOCK_TILE)
   || (newBottomTile == BLUE_DOOR_TILE && (!chip.blueKeys || isBlock))
   || (newBottomTile == RED_DOOR_TILE && (!chip.redKeys || isBlock))
   || (newBottomTile == GREEN_DOOR_TILE && (!chip.hasGreenKey || isBlock))
   || (newBottomTile == YELLOW_DOOR_TILE && (!chip.yellowKeys || isBlock))
   || (newBottomTile == SOCKET_TILE &&  (chipsLeft != 0 || isBlock))
   || ((deltaY  >  0  && newBottomTile == BLOCKED_NORTH_TILE) || (deltaY < 0 && bottomTile == BLOCKED_NORTH_TILE))
   || ((deltaX  >  0  && newBottomTile == BLOCKED_WEST_TILE) || (deltaX < 0 && bottomTile == BLOCKED_WEST_TILE))
   || ((deltaY  <  0  && newBottomTile == BLOCKED_SOUTH_TILE) || (deltaY > 0 && bottomTile == BLOCKED_SOUTH_TILE))
   || ((deltaX  <  0  && newBottomTile == BLOCKED_EAST_TILE) || (deltaX > 0 && bottomTile == BLOCKED_EAST_TILE))
   || (((deltaX > 0 || deltaY > 0) && bottomTile == BLOCKED_SOUTH_EAST_TILE) || ((deltaX < 0 || deltaY < 0) && newBottomTile == BLOCKED_SOUTH_EAST_TILE))

	|| (((deltaX < 0 || deltaY < 0) && bottomTile == SOUTH_EAST_ICE_TILE) || ((deltaX > 0 || deltaY > 0) && newBottomTile == SOUTH_EAST_ICE_TILE))
	|| (((deltaX > 0 || deltaY < 0) && bottomTile == SOUTH_WEST_ICE_TILE) || ((deltaX < 0 || deltaY > 0) && newBottomTile == SOUTH_WEST_ICE_TILE))
	|| (((deltaX > 0 || deltaY > 0) && bottomTile == NORTH_WEST_ICE_TILE) || ((deltaX < 0 || deltaY < 0) && newBottomTile == NORTH_WEST_ICE_TILE))
	|| (((deltaX < 0 || deltaY > 0) && bottomTile == NORTH_EAST_ICE_TILE) || ((deltaX > 0 || deltaY < 0) && newBottomTile == NORTH_EAST_ICE_TILE))))
		return true;

	for (unsigned int i = 0; i < map.layers.size(); i++)
	{
		int newtile = map.layers[i][x][y].get();
		if (newtile == 10 && (map.layers[0][oldX + (2 * deltaX)][oldY + (2 * deltaY)] == MUD_BLOCK_TILE
		|| isSolid(deltaX, deltaY, oldX + deltaX, oldY + deltaY, true))
		|| (newtile == 10 && isBlock)
		|| (isBlock && (newtile >= BUG_NORTH_TILE && newtile <= PARAMECIUM_EAST_TILE)))
			return true;
	}

	return false;
}

bool Game::isOpen(int x, int y)
{
	list<Trap>::iterator cur;

	for(cur = traps.begin(); cur != traps.end() && (cur->getTrapX() != x || cur->getTrapY() != y); cur++);

	if (cur != traps.end())
		return cur->getTrapX() == x && cur->getTrapY() == y && cur->isOpen;

	return false;
}

void Game::toggleOpen(int x, int y, bool status)
{
	list<Trap>::iterator cur;

	for(cur = traps.begin(); cur != traps.end() && (cur->getButtonX() != x || cur->getButtonY() != y); cur++);

	if (cur != traps.end())
		if (cur->getButtonX() == x && cur->getButtonY() == y)
			cur->isOpen = status;
}

bool Game::commonMovement(int x, int y, int deltaX, int deltaY, int& deltaX_v, int& deltaY_v, bool m, bool b)
{
	int bottomTile = bottomMostTile(x, y);

	// If the object is stuck in a trap or is the seed for a cloner
	if (bottomTile == CLONING_MACHINE_TILE || (bottomTile == TRAP_TILE && !isOpen(x, y)))
	{
		if (m == false && b == false) soundEffects["BlockedMoveSound"].play();
		deltaX_v = deltaY_v = 0;
		return false;
	}

	// If the object is stepping off a trap button
	if (bottomTile == BROWN_BUTTON_TILE)
		toggleOpen(x, y, false);

	// Switch to looking at the tile being stepped onto
	bottomTile = bottomMostTile(x + deltaX, y + deltaY);

	if (bottomTile == TELEPORT_TILE)
	{
		bool found = false;

		for (int i = x + deltaX - 1, j = y + deltaY; 
				 i != x + deltaX || j != y + deltaY; 
				 i = (i == -1) ? (j-- , 31) : i - 1, j = (j == -1) ? 31 : j)
		{
			int new_bottom = bottomMostTile(i, j);
			if (new_bottom == 41 && !((m == 0) ? isSolid(deltaX, deltaY, i, j, b)
											   : isSolid(deltaX, deltaY, i, j, m)))
			{
				// Go ahead and send the object through the teleport
				if (m == false && b == false) soundEffects["TeleportSound"].play();
				deltaX_v = i + deltaX - x;
				deltaY_v = j + deltaY - y;
				found = true;
				break;
			}
		}

		if (!found) // Something solid is on the other side
		{
			if (m == false && b == false) soundEffects["BlockedMoveSound"].play();
			deltaX_v = deltaY_v = 0;
			return false;
		}
	}

	// If the object steps on a trap button
	if (bottomTile == BROWN_BUTTON_TILE)
	{
		soundEffects["SwitchSound"].play();
		toggleOpen(x + deltaX, y + deltaY, true);
	}

	if (bottomTile == GREEN_BUTTON_TILE)
	{
		for (int i = 0; i < 32; i++)
		{
			for (int j = 0; j < 32; j++)
			{
				int index = bottomMostIndex(i, j);

				if (map.layers[index][i][j] == SWITCH_BLOCK_CLOSED_TILE)
					map.layers[index][i][j] = SWITCH_BLOCK_OPEN_TILE;
				else if (map.layers[index][i][j] == SWITCH_BLOCK_OPEN_TILE)
					map.layers[index][i][j] = SWITCH_BLOCK_CLOSED_TILE;
			}
		}
	}

	if (bottomTile == BLUE_BUTTON_TILE)
	{
		soundEffects["SwitchSound"].play();
		for (deque<Monster>::iterator cur = monsters.begin(); cur != monsters.end(); cur++)
		{
			if (cur->type == TANK && map.layers[bottomMostIndex(cur->x, cur->y)][cur->x][cur->y] != CLONING_MACHINE_TILE)
			{
				cur->currentDirection = (cur->currentDirection.toInt() < 3) ? cur->currentDirection.toInt() + 2 : cur->currentDirection.toInt() - 2;

				cur->canMove = true;
			}
		}
	}

	return true;
}

bool Game::handleClonerButton(int x, int y, int deltaX, int deltaY)
{
	int newBottomTile = bottomMostTile(x + deltaX, y + deltaY);

	if (monsters.size() >= 128)
		return false;

	if (newBottomTile == RED_BUTTON_TILE)
	{
		soundEffects["SwitchSound"].play();
		
		if (map.layers[0][x + deltaX][y + deltaY] > 67 && map.layers[0][x + deltaX][y + deltaY] < 72)
			int i = 4;

		int ndeltaX = 0, ndeltaY = 0;
		pair<int, int> cloner = cloners[make_pair(x + deltaX, y + deltaY)]; // Find cloner location
		Monster m;

		// Find monster/block at that location
		if (map.layers[0][cloner.first][cloner.second] > 13 && map.layers[0][cloner.first][cloner.second] < 18)
			m = Monster(cloner.first, cloner.second, map.layers[0][cloner.first][cloner.second].get() - 2);
		else if (map.layers[0][cloner.first][cloner.second] > 67 && map.layers[0][cloner.first][cloner.second] < 100)
			m = Monster(cloner.first, cloner.second, map.layers[0][cloner.first][cloner.second].get());
		else
			return false;

		Monster base = (m.type == TEETH || m.type == BUG || m.type == PARAMECIUM) ? monsters.back() : m;

		// Find monster orientation
		ndeltaX = base.currentDirection.deltaX();
		ndeltaY = base.currentDirection.deltaY();
		
		// If possible, create a new object there
		if ((m.type == BLOCK && !isSolid(ndeltaX, ndeltaY, m.x, m.y, true)) || !isSolid(ndeltaX, ndeltaY, m.x, m.y, m.type))
		{
			int tmp;

			if (m.type != BLOCK)
			{
				Monster m_new(m.x + ndeltaX, m.y + ndeltaY, (base.type * 4) + base.currentDirection.toInt()); // new monster
				monsters.push_back(m_new); // add to list
				tmp = (m_new.type * 4) + m_new.currentDirection.toInt();
			} else
				tmp = MUD_BLOCK_TILE;

			// Redraw tile
			redrawOldTile(tmp, m.x + ndeltaX, m.y + ndeltaY);
			map.layers[0][m.x + ndeltaX][m.y + ndeltaY] = tmp;

			if (tmp == MUD_BLOCK_TILE && isSlippery(m.x + ndeltaX, m.y + ndeltaY))
				movingBlocks.push_back(make_pair(make_pair(m.x + ndeltaX, m.y + ndeltaY), direction().set(ndeltaX, ndeltaY)));
		}

		return true;
	}

	return false;
}

void Game::handleChip()
{
	if (!isStarted && isActive)
		isStarted = true;

	bool blockToErase = false;
	int deltaX = 0, deltaY = 0, chipLayer = 0, bottomLayer = bottomMostIndex(chip.x, chip.y);
	direction forceDirection;

	// Find out which layer chip is on
	for (unsigned int i = 1; i < map.layers.size(); i++)
	{
		int tmp = map.layers[i][chip.x][chip.y].get();
		if ((tmp > 59 && tmp < 64) || tmp > 107)
			chipLayer = i;
	}

	// If chip is not on an ice or teleport block, only allow him to move selectively.
	if (!isSlippery(chip.x, chip.y, chip))
	{

		if (!(upKeyIsPressed || downKeyIsPressed || leftKeyIsPressed || rightKeyIsPressed) && recentKeyPresses.size() == 0 && chip.notForward)
		{
			chip.lastMove--;
			if (chip.lastMove == 0 && chip.notForward)
			{
				chip.notForward = false;
				moveChip(0, 0);
				DrawMap();
				return;
			}
		}

		if (chip.movedRecently && !chip.lastMoveWasForced) // Only allow Chip to move selectively
		{
			chip.movedRecently = false;
			return;
		}

		chip.movedRecently = true;

		if (!(upKeyIsPressed || downKeyIsPressed || leftKeyIsPressed || rightKeyIsPressed) && recentKeyPresses.size() == 0 && clickedPoint.x == 50)
			return;

		chip.lastMoveWasForced = false;

		if (recentKeyPresses.size() > 0 )
		{
			chip.currentDirection = recentKeyPresses.front();
			recentKeyPresses.pop();
			if (clickedPoint.x != 50)
				clickedPoint.x = clickedPoint.y = 50;
		}
		else
		{
			if (upKeyIsPressed)
				chip.currentDirection = UP;
			else if (downKeyIsPressed)
				chip.currentDirection = DOWN;
			else if (rightKeyIsPressed)
				chip.currentDirection = RIGHT;
			else if (leftKeyIsPressed)
				chip.currentDirection = LEFT;
			else
			{
				chip.currentDirection = direction();

				// Set up distance measurement variables
				int tempDeltaX = clickedPoint.x - chip.x;
				int tempDeltaY = clickedPoint.y - chip.y;
				deltaX = (tempDeltaX == 0) ? 0 : tempDeltaX / abs(tempDeltaX);
				deltaY = (tempDeltaY == 0) ? 0 : tempDeltaY / abs(tempDeltaY);

				bool xDirectionClear = !isSolid(deltaX, 0, chip.x, chip.y);
				bool yDirectionClear = !isSolid(0, deltaY, chip.x, chip.y);

				if (deltaY == 0 || (deltaY != 0 && deltaX != 0 && xDirectionClear && (!yDirectionClear || yDirectionClear && abs(tempDeltaY) < abs(tempDeltaX))))
					deltaY = 0;
				else
					deltaX = 0;

				if (isSolid(deltaX, deltaY, chip.x, chip.y))
					clickedPoint.x = clickedPoint.y = 50;

				if (deltaY == -1)
					chip.currentDirection = UP;
				if (deltaY == 1)
					chip.currentDirection = DOWN;
				if (deltaX == 1)
					chip.currentDirection = RIGHT;
				if (deltaX == -1)
					chip.currentDirection = LEFT;
			}
		}

		if (deltaX == 0 && deltaY == 0)
		{
			if (chip.currentDirection == UP)
				deltaY = -1;
			if (chip.currentDirection == DOWN)
				deltaY = 1;
			if (chip.currentDirection == RIGHT)
				deltaX = 1;
			if (chip.currentDirection == LEFT)
				deltaX = -1;
		}

		chip.notForward = true;

		moveChip(deltaX, deltaY);
		DrawMap();

	}
	else
	{
		bool force = true;

		function<void (direction&)> handleForceBlock = [&](direction& newDirection)
		{
			forceDirection = chip.currentDirection = newDirection;
			if (isSolid(deltaX = newDirection.deltaX(), deltaY = newDirection.deltaY(), chip.x, chip.y))
				soundEffects["BlockedMoveSound"].play();
			if (chip.lastMoveWasForced)
				force = !allowOverride(deltaX, deltaY);
		};

		function<void (direction&, bool, bool)> handleIceBlock = [&](direction& newDirection, bool isCurvedIceBlock, bool alternate)
		{
			int multiplier = (isCurvedIceBlock && alternate) || !isCurvedIceBlock ? -1 : 1;

			if (isSolid(deltaX = newDirection.deltaX(), deltaY = newDirection.deltaY(), chip.x, chip.y))
			{
				revealBlueWall(chip.x + deltaX, chip.y + deltaY);
				deltaX = (isCurvedIceBlock ? newDirection.deltaY() : newDirection.deltaX()) * multiplier;
				deltaY = (isCurvedIceBlock ? newDirection.deltaX() : newDirection.deltaY()) * multiplier;
				chip.lastDirection = chip.currentDirection = forceDirection.set(deltaX, deltaY);
				soundEffects["BlockedMoveSound"].play();
			}
			else
				chip.currentDirection.set(deltaX, deltaY);
		};

		switch(map.layers[bottomLayer][chip.x][chip.y].get())
		{
		case FORCE_SOUTH_TILE:
			handleForceBlock(direction(DOWN));
			break;
		case FORCE_NORTH_TILE:
			handleForceBlock(direction(UP));
			break;
		case FORCE_EAST_TILE:
			handleForceBlock(direction(RIGHT));
			break;
		case FORCE_WEST_TILE:
			handleForceBlock(direction(LEFT));
			break;
		case FORCE_ALL_DIRECTIONS_TILE: // force all directions
			handleForceBlock(direction(rand() % 4));
			break;
		case ICE_TILE: // Straight ice
			if (chip.lastDirection == UP)
				handleIceBlock(direction(UP), false, false);

			if (chip.lastDirection == DOWN)
				handleIceBlock(direction(DOWN), false, false);

			if (chip.lastDirection == LEFT)
				handleIceBlock(direction(LEFT), false, false);

			if (chip.lastDirection == RIGHT)
				handleIceBlock(direction(RIGHT), false, false);

			break;
		case SOUTH_EAST_ICE_TILE:
			if (chip.lastDirection == UP)
				handleIceBlock(direction(RIGHT), true, false);
			else
				handleIceBlock(direction(DOWN), true, false);
			break;
		case SOUTH_WEST_ICE_TILE:
			if (chip.lastDirection == UP)
				handleIceBlock(direction(LEFT), true, true);
			else
				handleIceBlock(direction(DOWN), true, true);
			break;
		case NORTH_WEST_ICE_TILE:
			if (chip.lastDirection == DOWN)
				handleIceBlock(direction(LEFT), true, false);
			else
				handleIceBlock(direction(UP), true, false);
			break;
		case NORTH_EAST_ICE_TILE:
			if (chip.lastDirection == DOWN)
				handleIceBlock(direction(RIGHT), true, true);
			else
				handleIceBlock(direction(UP), true, true);
			break;
		}

		if (force && !chip.lastMoveWasForced)
			chip.lastMoveWasForced = true;
		else if (!force && chip.lastMoveWasForced)
			chip.lastMoveWasForced = false;

		moveChip(deltaX, deltaY);
		DrawMap();
	}

	chip.lastDirection.set(deltaX, deltaY);
}

bool Game::allowOverride(int& deltaX, int& deltaY)
{
	int bottomTile = map.layers[bottomMostIndex(chip.x, chip.y)][chip.x][chip.y].get();

	// Chip isn't always supposed to be forced in the force block.
	if ((bottomTile >= FORCE_NORTH_TILE && bottomTile <= FORCE_WEST_TILE) || bottomTile == FORCE_SOUTH_TILE)
	{
		if (recentKeyPresses.size() > 0)
		{
			chip.currentDirection = recentKeyPresses.front();
			recentKeyPresses.pop();
		}
		else
		{
			if (upKeyIsPressed)
				chip.currentDirection = UP;

			if (downKeyIsPressed)
				chip.currentDirection = DOWN;

			if (rightKeyIsPressed)
				chip.currentDirection = RIGHT;

			if (leftKeyIsPressed)
				chip.currentDirection = LEFT;
		}

		if (upKeyIsPressed && bottomTile == FORCE_SOUTH_TILE)
		{
			deltaX = deltaY = 0;
			return false;
		}

		if (downKeyIsPressed && bottomTile == FORCE_NORTH_TILE)
		{
			deltaX = deltaY = 0;
			return false;
		}

		if (rightKeyIsPressed && bottomTile == FORCE_WEST_TILE)
		{
			deltaX = deltaY = 0;
			return false;
		}

		if (leftKeyIsPressed && bottomTile == FORCE_EAST_TILE)
		{
			deltaX = deltaY = 0;
			return false;
		}

		deltaX = chip.currentDirection.deltaX();
		deltaY = chip.currentDirection.deltaY();
		return true;
	}

	return false;
}