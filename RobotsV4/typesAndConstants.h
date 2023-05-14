//
//  typesAndConstants.h
//  GL threads
//

#ifndef TYPES_AND_CONSTANTS_H
#define TYPES_AND_CONSTANTS_H

#include <vector>
#include <mutex>

//===============================================
//	Application-wide constants
//===============================================
inline constexpr int 	MIN_SLEEP_TIME = 1000;

inline constexpr int	GRID_PANE_WIDTH = 900,
						GRID_PANE_HEIGHT = GRID_PANE_WIDTH,
						STATE_PANE_WIDTH = 300,
						STATE_PANE_HEIGHT = GRID_PANE_HEIGHT,
						H_PADDING = 5,
						WINDOW_WIDTH = GRID_PANE_WIDTH + STATE_PANE_WIDTH + H_PADDING,
						WINDOW_HEIGHT = GRID_PANE_HEIGHT;


//===============================================
//	Custom data types
//===============================================

//	Travel direction data type
enum Direction {
					NORTH = 0,
					WEST,
					SOUTH,
					EAST,
					//
					NUM_TRAVEL_DIRECTIONS
};

enum FontSize {
					SMALL_FONT_SIZE,
					MEDIUM_FONT_SIZE,
					LARGE_FONT_SIZE,
					//
					NUM_FONT_SIZES
};

enum class SquareType
{
	FREE_SQUARE,
	DOOR,
	ROBOT,
	BOX,
	VERTICAL_PARTITION,
	HORIZONTAL_PARTITION,
	//
	NUM_SQUARE_TYPES

};

/**	Data type to store the position of *things* on the grid
 */
struct GridPosition
{
	/**	row index
	 */
	unsigned int row;
	/** column index
	 */
	unsigned int col;

};

/**
 *	Data type to represent a sliding partition
 */
struct SlidingPartition
{
	/*	vertical vs. horizontal partition
	 */
	bool isVertical;

	/**	The blocks making up the partition, listed
	 *		top-to-bottom for a vertical list
	 *		left-to-right for a horizontal list
	 */
	std::vector<GridPosition> blockList;

};

/**
 *	Data type to represent each robot information needed to work
 */
struct RobotInfo
{
	int id;
	int* row;
	int* col;

	int* boxRow;
	int* boxCol;

	int doorId;
	int* doorRow;
	int* doorCol;

	bool isPushing;

	int moveCounter;
	int pushIndex;

	std::vector<std::pair<char, int>> plan;
	std::vector<std::pair<char, int>> boxPlan;

	bool isAlive;
};

#endif //	TYPES_AND_CONSTANTS_H
