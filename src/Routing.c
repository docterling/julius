#include "Routing.h"

#include "TerrainGraphics.h"

#include "Data/Building.h"
#include "Data/Constants.h"
#include "Data/Grid.h"
#include "Data/Routes.h"
#include "Data/Figure.h"
#include "Data/State.h"

#include "core/calc.h"
#include "core/random.h"
#include "graphics/image.h"
#include "map/grid.h"

#define MAX_QUEUE 26244

static struct {
	int head;
	int tail;
	int items[MAX_QUEUE];
} queue;

static struct {
	int throughBuildingId;
	int isAqueduct;
} state;

static int directionPath[500];

static int8_t tmpGrid[GRID_SIZE * GRID_SIZE];

static void setDistAndEnqueue(int nextOffset, int dist)
{
	Data_Grid_routingDistance[nextOffset] = dist;
	queue.items[queue.tail++] = nextOffset;
	if (queue.tail >= MAX_QUEUE) queue.tail = 0;
}

static void routeQueue(int source, int dest, void (*callback)(int nextOffset, int dist))
{
	map_grid_clear_u16(Data_Grid_routingDistance);
	Data_Grid_routingDistance[source] = 1;
	queue.items[0] = source;
	queue.head = 0;
	queue.tail = 1;
	while (queue.head != queue.tail) {
		int offset = queue.items[queue.head];
		if (offset == dest) break;
		int dist = 1 + Data_Grid_routingDistance[offset];
		int nextOffset = offset - 162;
		if (nextOffset >= 0 && !Data_Grid_routingDistance[nextOffset]) {
			callback(nextOffset, dist);
		}
		nextOffset = offset + 1;
		if (nextOffset < 162 * 162 && !Data_Grid_routingDistance[nextOffset]) {
			callback(nextOffset, dist);
		}
		nextOffset = offset + 162;
		if (nextOffset < 162 * 162 && !Data_Grid_routingDistance[nextOffset]) {
			callback(nextOffset, dist);
		}
		nextOffset = offset - 1;
		if (nextOffset >= 0 && !Data_Grid_routingDistance[nextOffset]) {
			callback(nextOffset, dist);
		}
		if (++queue.head >= MAX_QUEUE) queue.head = 0;
	}
}

static void routeQueueWhileTrue(int source, int (*callback)(int nextOffset, int dist))
{
	map_grid_clear_u16(Data_Grid_routingDistance);
	Data_Grid_routingDistance[source] = 1;
	queue.items[0] = source;
	queue.head = 0;
	queue.tail = 1;
	while (queue.head != queue.tail) {
		int offset = queue.items[queue.head];
		int dist = 1 + Data_Grid_routingDistance[offset];
		int nextOffset = offset - 162;
		if (nextOffset >= 0 && !Data_Grid_routingDistance[nextOffset]) {
			if (!callback(nextOffset, dist)) break;
		}
		nextOffset = offset + 1;
		if (nextOffset < 162 * 162 && !Data_Grid_routingDistance[nextOffset]) {
			if (!callback(nextOffset, dist)) break;
		}
		nextOffset = offset + 162;
		if (nextOffset < 162 * 162 && !Data_Grid_routingDistance[nextOffset]) {
			if (!callback(nextOffset, dist)) break;
		}
		nextOffset = offset - 1;
		if (nextOffset >= 0 && !Data_Grid_routingDistance[nextOffset]) {
			if (!callback(nextOffset, dist)) break;
		}
		if (++queue.head >= MAX_QUEUE) queue.head = 0;
	}
}

static void routeQueueMax(int source, int dest, int maxTiles, void (*callback)(int, int))
{
	map_grid_clear_u16(Data_Grid_routingDistance);
	Data_Grid_routingDistance[source] = 1;
	queue.items[0] = source;
	queue.head = 0;
	queue.tail = 1;
	int tiles = 0;
	while (queue.head != queue.tail) {
		int offset = queue.items[queue.head];
		if (offset == dest) break;
		if (++tiles > maxTiles) break;
		int dist = 1 + Data_Grid_routingDistance[offset];
		int nextOffset = offset - 162;
		if (nextOffset >= 0 && !Data_Grid_routingDistance[nextOffset]) {
			callback(nextOffset, dist);
		}
		nextOffset = offset + 1;
		if (nextOffset < 162 * 162 && !Data_Grid_routingDistance[nextOffset]) {
			callback(nextOffset, dist);
		}
		nextOffset = offset + 162;
		if (nextOffset < 162 * 162 && !Data_Grid_routingDistance[nextOffset]) {
			callback(nextOffset, dist);
		}
		nextOffset = offset - 1;
		if (nextOffset >= 0 && !Data_Grid_routingDistance[nextOffset]) {
			callback(nextOffset, dist);
		}
		if (++queue.head >= MAX_QUEUE) queue.head = 0;
	}
}

static void routeQueueBoat(int source, void (*callback)(int, int))
{
	map_grid_clear_u16(Data_Grid_routingDistance);
	map_grid_clear_i8(tmpGrid);
	Data_Grid_routingDistance[source] = 1;
	queue.items[0] = source;
	queue.head = 0;
	queue.tail = 1;
	int tiles = 0;
	while (queue.head != queue.tail) {
		int offset = queue.items[queue.head];
		if (++tiles > 50000) break;
		int drag = Data_Grid_routingWater[offset] == Routing_Water_m2_MapEdge ? 4 : 0;
		if (drag && tmpGrid[offset]++ < drag) {
			queue.items[queue.tail++] = offset;
			if (queue.tail >= MAX_QUEUE) queue.tail = 0;
		} else {
			int dist = 1 + Data_Grid_routingDistance[offset];
			int nextOffset = offset - 162;
			if (nextOffset >= 0 && !Data_Grid_routingDistance[nextOffset]) {
				callback(nextOffset, dist);
			}
			nextOffset = offset + 1;
			if (nextOffset < 162 * 162 && !Data_Grid_routingDistance[nextOffset]) {
				callback(nextOffset, dist);
			}
			nextOffset = offset + 162;
			if (nextOffset < 162 * 162 && !Data_Grid_routingDistance[nextOffset]) {
				callback(nextOffset, dist);
			}
			nextOffset = offset - 1;
			if (nextOffset >= 0 && !Data_Grid_routingDistance[nextOffset]) {
				callback(nextOffset, dist);
			}
		}
		if (++queue.head >= MAX_QUEUE) queue.head = 0;
	}
}

static void routeQueueDir8(int source, void (*callback)(int, int))
{
	map_grid_clear_u16(Data_Grid_routingDistance);
	Data_Grid_routingDistance[source] = 1;
	queue.items[0] = source;
	queue.head = 0;
	queue.tail = 1;
	int tiles = 0;
	while (queue.head != queue.tail) {
		if (++tiles > 50000) break;
		int offset = queue.items[queue.head];
		int dist = 1 + Data_Grid_routingDistance[offset];
		int nextOffset = offset - 162;
		if (nextOffset >= 0 && !Data_Grid_routingDistance[nextOffset]) {
			callback(nextOffset, dist);
		}
		nextOffset = offset + 1;
		if (nextOffset < 162 * 162 && !Data_Grid_routingDistance[nextOffset]) {
			callback(nextOffset, dist);
		}
		nextOffset = offset + 162;
		if (nextOffset < 162 * 162 && !Data_Grid_routingDistance[nextOffset]) {
			callback(nextOffset, dist);
		}
		nextOffset = offset - 1;
		if (nextOffset >= 0 && !Data_Grid_routingDistance[nextOffset]) {
			callback(nextOffset, dist);
		}
		nextOffset = offset - 161;
		if (nextOffset >= 0 && !Data_Grid_routingDistance[nextOffset]) {
			callback(nextOffset, dist);
		}
		nextOffset = offset + 163;
		if (nextOffset < 162 * 162 && !Data_Grid_routingDistance[nextOffset]) {
			callback(nextOffset, dist);
		}
		nextOffset = offset + 161;
		if (nextOffset < 162 * 162 && !Data_Grid_routingDistance[nextOffset]) {
			callback(nextOffset, dist);
		}
		nextOffset = offset - 163;
		if (nextOffset >= 0 && !Data_Grid_routingDistance[nextOffset]) {
			callback(nextOffset, dist);
		}
		if (++queue.head >= MAX_QUEUE) queue.head = 0;
	}
}

static void callbackGetDistance(int nextOffset, int dist)
{
	if (Data_Grid_routingLandCitizen[nextOffset] >= Routing_Citizen_0_Road) {
		setDistAndEnqueue(nextOffset, dist);
	}
}

void Routing_getDistance(int x, int y)
{
	int sourceOffset = GridOffset(x, y);
	++Data_Routes.totalRoutesCalculated;
	routeQueue(sourceOffset, -1, callbackGetDistance);
}

static int callbackDeleteClosestWallOrAqueduct(int nextOffset, int dist)
{
	if (Data_Grid_routingLandCitizen[nextOffset] < Routing_Citizen_0_Road) {
		if (Data_Grid_terrain[nextOffset] & (Terrain_Aqueduct | Terrain_Wall)) {
			Data_Grid_terrain[nextOffset] &= Terrain_2e80;
			return 0;
		}
	} else {
		setDistAndEnqueue(nextOffset, dist);
	}
	return 1;
}

void Routing_deleteClosestWallOrAqueduct(int x, int y)
{
	int sourceOffset = GridOffset(x, y);
	++Data_Routes.totalRoutesCalculated;
	routeQueueWhileTrue(sourceOffset, callbackDeleteClosestWallOrAqueduct);
}

static int hasFightingFriendly(int gridOffset)
{
	int figureId = Data_Grid_figureIds[gridOffset];
	if (figureId > 0) {
		while (figureId) {
			if (Data_Figures[figureId].isFriendly &&
				Data_Figures[figureId].actionState == FigureActionState_150_Attack) {
				return 1;
			}
			figureId = Data_Figures[figureId].nextFigureIdOnSameTile;
		}
	}
	return 0;
}

static int hasFightingEnemy(int gridOffset)
{
	int figureId = Data_Grid_figureIds[gridOffset];
	if (figureId > 0) {
		while (figureId) {
			if (!Data_Figures[figureId].isFriendly &&
				Data_Figures[figureId].actionState == FigureActionState_150_Attack) {
				return 1;
			}
			figureId = Data_Figures[figureId].nextFigureIdOnSameTile;
		}
	}
	return 0;
}

static void callbackCanTravelOverLandCitizen(int nextOffset, int dist)
{
	if (Data_Grid_routingLandCitizen[nextOffset] >= 0 && !hasFightingFriendly(nextOffset)) {
		setDistAndEnqueue(nextOffset, dist);
	}
}

int Routing_canTravelOverLandCitizen(int xSrc, int ySrc, int xDst, int yDst)
{
	int sourceOffset = GridOffset(xSrc, ySrc);
	int destOffset = GridOffset(xDst, yDst);
	++Data_Routes.totalRoutesCalculated;
	routeQueue(sourceOffset, destOffset, callbackCanTravelOverLandCitizen);
	return Data_Grid_routingDistance[destOffset] != 0;
}

static void callbackCanTravelOverRoadGardenCitizen(int nextOffset, int dist)
{
	if (Data_Grid_routingLandCitizen[nextOffset] >= Routing_Citizen_0_Road &&
		Data_Grid_routingLandCitizen[nextOffset] <= Routing_Citizen_2_PassableTerrain) {
		setDistAndEnqueue(nextOffset, dist);
	}
}

int Routing_canTravelOverRoadGardenCitizen(int xSrc, int ySrc, int xDst, int yDst)
{
	int sourceOffset = GridOffset(xSrc, ySrc);
	int destOffset = GridOffset(xDst, yDst);
	++Data_Routes.totalRoutesCalculated;
	routeQueue(sourceOffset, destOffset, callbackCanTravelOverRoadGardenCitizen);
	return Data_Grid_routingDistance[destOffset] != 0;
}

static void callbackCanTravelOverWalls(int nextOffset, int dist)
{
	if (Data_Grid_routingWalls[nextOffset] >= Routing_Wall_0_Passable &&
		Data_Grid_routingWalls[nextOffset] <= 2) {
		setDistAndEnqueue(nextOffset, dist);
	}
}

int Routing_canTravelOverWalls(int xSrc, int ySrc, int xDst, int yDst)
{
	int sourceOffset = GridOffset(xSrc, ySrc);
	int destOffset = GridOffset(xDst, yDst);
	++Data_Routes.totalRoutesCalculated;
	routeQueue(sourceOffset, destOffset, callbackCanTravelOverWalls);
	return Data_Grid_routingDistance[destOffset] != 0;
}

static void callbackCanTravelOverLandNonCitizenThroughBuilding(int nextOffset, int dist)
{
	if (!hasFightingEnemy(nextOffset)) {
		if (Data_Grid_routingLandNonCitizen[nextOffset] == Routing_NonCitizen_0_Passable ||
			Data_Grid_routingLandNonCitizen[nextOffset] == Routing_NonCitizen_2_Clearable ||
			(Data_Grid_routingLandNonCitizen[nextOffset] == Routing_NonCitizen_1_Building &&
				Data_Grid_buildingIds[nextOffset] == state.throughBuildingId)) {
			setDistAndEnqueue(nextOffset, dist);
		}
	}
}

static void callbackCanTravelOverLandNonCitizen(int nextOffset, int dist)
{
	if (!hasFightingEnemy(nextOffset)) {
		if (Data_Grid_routingLandNonCitizen[nextOffset] >= Routing_NonCitizen_0_Passable &&
			Data_Grid_routingLandNonCitizen[nextOffset] < Routing_NonCitizen_5_Fort) {
			setDistAndEnqueue(nextOffset, dist);
		}
	}
}

int Routing_canTravelOverLandNonCitizen(int xSrc, int ySrc, int xDst, int yDst, int onlyThroughBuildingId, int maxTiles)
{
	int sourceOffset = GridOffset(xSrc, ySrc);
	int destOffset = GridOffset(xDst, yDst);
	++Data_Routes.totalRoutesCalculated;
	++Data_Routes.enemyRoutesCalculated;
	if (onlyThroughBuildingId) {
		state.throughBuildingId = onlyThroughBuildingId;
		routeQueue(sourceOffset, destOffset, callbackCanTravelOverLandNonCitizenThroughBuilding);
	} else {
		routeQueueMax(sourceOffset, destOffset, maxTiles, callbackCanTravelOverLandNonCitizen);
	}
	return Data_Grid_routingDistance[destOffset] != 0;
}

static void callbackCanTravelThroughEverythingNonCitizen(int nextOffset, int dist)
{
	if (Data_Grid_routingLandNonCitizen[nextOffset] >= Routing_NonCitizen_0_Passable) {
		setDistAndEnqueue(nextOffset, dist);
	}
}

int Routing_canTravelThroughEverythingNonCitizen(int xSrc, int ySrc, int xDst, int yDst)
{
	int sourceOffset = GridOffset(xSrc, ySrc);
	int destOffset = GridOffset(xDst, yDst);
	++Data_Routes.totalRoutesCalculated;
	routeQueue(sourceOffset, destOffset, callbackCanTravelThroughEverythingNonCitizen);
	return Data_Grid_routingDistance[destOffset] != 0;
}

int Routing_canPlaceRoadUnderAqueduct(int gridOffset)
{
	int graphic = Data_Grid_graphicIds[gridOffset] - image_group(GROUP_BUILDING_AQUEDUCT);
	int checkRoadY;
	switch (graphic) {
		case 0:
		case 2:
		case 8:
		case 15:
		case 17:
		case 23:
			checkRoadY = 1;
			break;
		case 1:
		case 3:
		case 9: case 10: case 11: case 12: case 13: case 14:
		case 16:
		case 18:
		case 24: case 25: case 26: case 27: case 28: case 29:
			checkRoadY = 0;
			break;
		default: // not a straight aqueduct
			return 0;
	}
	if (Data_State.map.orientation == Dir_6_Left || Data_State.map.orientation == Dir_2_Right) {
		checkRoadY = !checkRoadY;
	}
	if (checkRoadY) {
		if ((Data_Grid_terrain[gridOffset - 162] & Terrain_Road) ||
			Data_Grid_routingDistance[gridOffset - 162] > 0) {
			return 0;
		}
		if ((Data_Grid_terrain[gridOffset + 162] & Terrain_Road) ||
			Data_Grid_routingDistance[gridOffset + 162] > 0) {
			return 0;
		}
	} else {
		if ((Data_Grid_terrain[gridOffset - 1] & Terrain_Road) ||
			Data_Grid_routingDistance[gridOffset - 1] > 0) {
			return 0;
		}
		if ((Data_Grid_terrain[gridOffset + 1] & Terrain_Road) ||
			Data_Grid_routingDistance[gridOffset + 1] > 0) {
			return 0;
		}
	}
	return 1;
}

int Routing_getAqueductGraphicOffsetWithRoad(int gridOffset)
{
	int graphic = Data_Grid_graphicIds[gridOffset] - image_group(GROUP_BUILDING_AQUEDUCT);
	switch (graphic) {
		case 2:
			return 8;
		case 3:
			return 9;
		case 0:
		case 1:
		case 8:
		case 9:
		case 15:
		case 16:
		case 17:
		case 18:
		case 23:
		case 24:
			// unchanged
			return graphic;
		default:
			// shouldn't happen
			return 8;
	}
}

static int canPlaceAqueductOnRoad(int gridOffset)
{
	int graphic = Data_Grid_graphicIds[gridOffset] - image_group(GROUP_TERRAIN_ROAD);
	if (graphic != 0 && graphic != 1 && graphic != 49 && graphic != 50) {
		return 0;
	}
	int checkRoadY = graphic == 0 || graphic == 49;
	if (Data_State.map.orientation == Dir_6_Left || Data_State.map.orientation == Dir_2_Right) {
		checkRoadY = !checkRoadY;
	}
	if (checkRoadY) {
		if (Data_Grid_routingDistance[gridOffset - 162] > 0 ||
			Data_Grid_routingDistance[gridOffset + 162] > 0) {
			return 0;
		}
	} else {
		if (Data_Grid_routingDistance[gridOffset - 1] > 0 ||
			Data_Grid_routingDistance[gridOffset + 1] > 0) {
			return 0;
		}
	}
	return 1;
}

static int canPlaceInitialRoadOrAqueduct(int gridOffset, int isAqueduct)
{
	if (Data_Grid_routingLandCitizen[gridOffset] == Routing_Citizen_m1_Blocked) {
		// not open land, can only if:
		// - aqueduct should be placed, and:
		// - land is a reservoir building OR an aqueduct
		if (!isAqueduct) {
			return 0;
		}
		if (Data_Grid_terrain[gridOffset] & Terrain_Aqueduct) {
			return 1;
		}
		if (Data_Grid_terrain[gridOffset] & Terrain_Building) {
			if (Data_Buildings[Data_Grid_buildingIds[gridOffset]].type == BUILDING_RESERVOIR) {
				return 1;
			}
		}
		return 0;
	} else if (Data_Grid_routingLandCitizen[gridOffset] == Routing_Citizen_2_PassableTerrain) {
		// rubble, access ramp, garden
		return 0;
	} else if (Data_Grid_routingLandCitizen[gridOffset] == Routing_Citizen_m3_Aqueduct) {
		if (isAqueduct) {
			return 0;
		}
		if (Routing_canPlaceRoadUnderAqueduct(gridOffset)) {
			return 1;
		}
		return 0;
	} else {
		return 1;
	}
}

static void callbackGetDistanceForBuildingRoadOrAqueduct(int nextOffset, int dist)
{
	int blocked = 0;
	switch (Data_Grid_routingLandCitizen[nextOffset]) {
		case Routing_Citizen_m3_Aqueduct:
			if (state.isAqueduct) {
				blocked = 1;
			} else if (!Routing_canPlaceRoadUnderAqueduct(nextOffset)) {
				Data_Grid_routingDistance[nextOffset] = -1;
				blocked = 1;
			}
			break;
		case Routing_Citizen_2_PassableTerrain: // rubble, garden, access ramp
		case Routing_Citizen_m1_Blocked: // non-empty land
			blocked = 1;
			break;
		default:
			if (Data_Grid_terrain[nextOffset] & Terrain_Building) {
				if (Data_Grid_routingLandCitizen[nextOffset] != Routing_Citizen_m4_ReservoirConnector ||
					!state.isAqueduct) {
					blocked = 1;
				}
			}
			break;
	}
	if (Data_Grid_terrain[nextOffset] & Terrain_Road) {
		if (state.isAqueduct && !canPlaceAqueductOnRoad(nextOffset)) {
			Data_Grid_routingDistance[nextOffset] = -1;
			blocked = 1;
		}
	}
	if (!blocked) {
		setDistAndEnqueue(nextOffset, dist);
	}
}

int Routing_getDistanceForBuildingRoadOrAqueduct(int x, int y, int isAqueduct)
{
	int sourceOffset = GridOffset(x, y);
	if (!canPlaceInitialRoadOrAqueduct(sourceOffset, isAqueduct)) {
		return 0;
	}
	if (Data_Grid_terrain[sourceOffset] & Terrain_Road &&
		isAqueduct && !canPlaceAqueductOnRoad(sourceOffset)) {
		return 0;
	}
	++Data_Routes.totalRoutesCalculated;
	state.isAqueduct = isAqueduct;
	routeQueue(sourceOffset, -1, callbackGetDistanceForBuildingRoadOrAqueduct);
	return 1;
}

static void callbackGetDistanceForBuildingWall(int nextOffset, int dist)
{
	if (Data_Grid_routingLandCitizen[nextOffset] == Routing_Citizen_4_ClearTerrain) {
		setDistAndEnqueue(nextOffset, dist);
	}
}

int Routing_getDistanceForBuildingWall(int x, int y)
{
	int sourceOffset = GridOffset(x, y);
	routeQueue(sourceOffset, -1, callbackGetDistanceForBuildingWall);
	return 1;
}

int Routing_placeRoutedBuilding(int xSrc, int ySrc, int xDst, int yDst, RoutedBuilding type, int *items)
{
	static const int directionIndices[8][4] = {
		{0, 2, 6, 4},
		{0, 2, 6, 4},
		{2, 4, 0, 6},
		{2, 4, 0, 6},
		{4, 6, 2, 0},
		{4, 6, 2, 0},
		{6, 0, 4, 2},
		{6, 0, 4, 2}
	};
	*items = 0;
	int gridOffset = GridOffset(xDst, yDst);
	int guard = 0;
	// reverse routing
	while (1) {
		if (++guard >= 400) {
			return 0;
		}
		int distance = Data_Grid_routingDistance[gridOffset];
		if (distance <= 0) {
			return 0;
		}
		switch (type) {
			default:
			case RoutedBUILDING_ROAD:
				*items += TerrainGraphics_setTileRoad(xDst, yDst);
				break;
			case RoutedBUILDING_WALL:
				*items += TerrainGraphics_setTileWall(xDst, yDst);
				break;
			case RoutedBuilding_Aqueduct:
				*items += TerrainGraphics_setTileAqueductTerrain(xDst, yDst);
				break;
			case RoutedBuilding_AqueductWithoutGraphic:
				*items += 1;
				break;
		}
		int direction = calc_general_direction(xDst, yDst, xSrc, ySrc);
		if (direction == Dir_8_None) {
			return 1; // destination reached
		}
		int routed = 0;
		for (int i = 0; i < 4; i++) {
			int index = directionIndices[direction][i];
			int newGridOffset = gridOffset + Constant_DirectionGridOffsets[index];
			int newDist = Data_Grid_routingDistance[newGridOffset];
			if (newDist > 0 && newDist < distance) {
				gridOffset = newGridOffset;
				xDst = GridOffsetToX(gridOffset);
				yDst = GridOffsetToY(gridOffset);
				routed = 1;
				break;
			}
		}
		if (!routed) {
			return 0;
		}
	}
}

static void updateXYGridOffsetForDirection(int direction, int *x, int *y, int *gridOffset)
{
	switch (direction) {
		case Dir_0_Top:
			--(*y);
			(*gridOffset) -= 162;
			break;
		case Dir_1_TopRight:
			++(*x);
			--(*y);
			(*gridOffset) -= 161;
			break;
		case Dir_2_Right:
			++(*x);
			++(*gridOffset);
			break;
		case Dir_3_BottomRight:
			++(*x);
			++(*y);
			(*gridOffset) += 163;
			break;
		case Dir_4_Bottom:
			++(*y);
			(*gridOffset) += 162;
			break;
		case Dir_5_BottomLeft:
			--(*x);
			++(*y);
			(*gridOffset) += 161;
			break;
		case Dir_6_Left:
			--(*x);
			--(*gridOffset);
			break;
		case Dir_7_TopLeft:
			--(*x);
			--(*y);
			(*gridOffset) -= 163;
			break;
	}
}

static void callbackGetDistanceWaterBoat(int nextOffset, int dist)
{
	if (Data_Grid_routingWater[nextOffset] != Routing_Water_m1_Blocked &&
		Data_Grid_routingWater[nextOffset] != Routing_Water_m3_LowBridge) {
		setDistAndEnqueue(nextOffset, dist);
		if (Data_Grid_routingWater[nextOffset] == Routing_Water_m2_MapEdge) {
			Data_Grid_routingDistance[nextOffset] += 4;
		}
	}
}

void Routing_getDistanceWaterBoat(int x, int y)
{
	int sourceGridOffset = GridOffset(x, y);
	if (Data_Grid_routingWater[sourceGridOffset] == Routing_Water_m1_Blocked) {
		return;
	}
	routeQueueBoat(sourceGridOffset, callbackGetDistanceWaterBoat);
}

static void callbackGetDistanceWaterFlotsam(int nextOffset, int dist)
{
	if (Data_Grid_routingWater[nextOffset] != Routing_Water_m1_Blocked) {
		setDistAndEnqueue(nextOffset, dist);
	}
}

void Routing_getDistanceWaterFlotsam(int x, int y)
{
	int sourceGridOffset = GridOffset(x, y);
	if (Data_Grid_routingWater[sourceGridOffset] == Routing_Water_m1_Blocked) {
		return;
	}
	routeQueueDir8(sourceGridOffset, callbackGetDistanceWaterFlotsam);
}

int Routing_getPath(uint8_t *path, int xSrc, int ySrc, int xDst, int yDst, int numDirections)
{
	int dstGridOffset = GridOffset(xDst, yDst);
	int distance = Data_Grid_routingDistance[dstGridOffset];
	if (distance <= 0 || distance >= 998) {
		return 0;
	}

	int numTiles = 0;
	int lastDirection = -1;
	int x = xDst;
	int y = yDst;
	int gridOffset = dstGridOffset;
	int step = numDirections == 8 ? 1 : 2;

	while (distance > 1) {
		distance = Data_Grid_routingDistance[gridOffset];
		int direction = -1;
		int generalDirection = calc_general_direction(x, y, xSrc, ySrc);
		for (int d = 0; d < 8; d += step) {
			if (d != lastDirection) {
				int nextOffset = gridOffset + Constant_DirectionGridOffsets[d];
				int nextDistance = Data_Grid_routingDistance[nextOffset];
				if (nextDistance) {
					if (nextDistance < distance) {
						distance = nextDistance;
						direction = d;
					} else if (nextDistance == distance && (d == generalDirection || direction == -1)) {
						distance = nextDistance;
						direction = d;
					}
				}
			}
		}
		if (direction == -1) {
			return 0;
		}
		updateXYGridOffsetForDirection(direction, &x, &y, &gridOffset);
		int forwardDirection = (direction + 4) % 8;
		directionPath[numTiles++] = forwardDirection;
		lastDirection = forwardDirection;
		if (numTiles >= 500) {
			return 0;
		}
	}
	for (int i = 0; i < numTiles; i++) {
		path[i] = directionPath[numTiles - i - 1];
	}
	return numTiles;
}

int Routing_getClosestXYWithinRange(int numDirections, int xSrc, int ySrc, int xDst, int yDst, int range, int *xOut, int *yOut)
{
	int dstGridOffset = GridOffset(xDst, yDst);
	int distance = Data_Grid_routingDistance[dstGridOffset];
	if (distance <= 0 || distance >= 998) {
		return 0;
	}

	int numTiles = 0;
	int lastDirection = -1;
	int x = xDst;
	int y = yDst;
	int gridOffset = dstGridOffset;
	int step = numDirections == 8 ? 1 : 2;

	while (distance > 1) {
		distance = Data_Grid_routingDistance[gridOffset];
		*xOut = x;
		*yOut = y;
		if (distance <= range) {
			return 1;
		}
		int direction = -1;
		int generalDirection = calc_general_direction(x, y, xSrc, ySrc);
		for (int d = 0; d < 8; d += step) {
			if (d != lastDirection) {
				int nextOffset = gridOffset + Constant_DirectionGridOffsets[d];
				int nextDistance = Data_Grid_routingDistance[nextOffset];
				if (nextDistance) {
					if (nextDistance < distance) {
						distance = nextDistance;
						direction = d;
					} else if (nextDistance == distance && (d == generalDirection || direction == -1)) {
						distance = nextDistance;
						direction = d;
					}
				}
			}
		}
		if (direction == -1) {
			return 0;
		}
		updateXYGridOffsetForDirection(direction, &x, &y, &gridOffset);
		int forwardDirection = (direction + 4) % 8;
		directionPath[numTiles++] = forwardDirection;
		lastDirection = forwardDirection;
		if (numTiles >= 500) {
			return 0;
		}
	}
	return 0;
}

int Routing_getPathOnWater(uint8_t *path, int xSrc, int ySrc, int xDst, int yDst, int isFlotsam)
{
	int rand = random_byte() & 3;
	int dstGridOffset = GridOffset(xDst, yDst);
	int distance = Data_Grid_routingDistance[dstGridOffset];
	if (distance <= 0 || distance >= 998) {
		return 0;
	}

	int numTiles = 0;
	int lastDirection = -1;
	int x = xDst;
	int y = yDst;
	int gridOffset = dstGridOffset;
	while (distance > 1) {
		int currentRand = rand;
		distance = Data_Grid_routingDistance[gridOffset];
		if (isFlotsam) {
			currentRand = Data_Grid_random[gridOffset] & 3;
		}
		int direction = -1;
		for (int d = 0; d < 8; d++) {
			if (d != lastDirection) {
				int nextOffset = gridOffset + Constant_DirectionGridOffsets[d];
				int nextDistance = Data_Grid_routingDistance[nextOffset];
				if (nextDistance) {
					if (nextDistance < distance) {
						distance = nextDistance;
						direction = d;
					} else if (nextDistance == distance && rand == currentRand) {
						// allow flotsam to wander
						distance = nextDistance;
						direction = d;
					}
				}
			}
		}
		if (direction == -1) {
			return 0;
		}
		updateXYGridOffsetForDirection(direction, &x, &y, &gridOffset);
		int forwardDirection = (direction + 4) % 8;
		directionPath[numTiles++] = forwardDirection;
		lastDirection = forwardDirection;
		if (numTiles >= 500) {
			return 0;
		}
	}
	for (int i = 0; i < numTiles; i++) {
		path[i] = directionPath[numTiles - i - 1];
	}
	return numTiles;
}

void Routing_block(int x, int y, int size)
{
	if (IsOutsideMap(x, y, size)) {
		return;
	}
	for (int dy = 0; dy < size; dy++) {
		for (int dx = 0; dx < size; dx++) {
			Data_Grid_routingDistance[GridOffset(x+dx, y+dy)] = 0;
		}
	}
}
