(function () {
	// GAME CONFIGURATION
	var beginRate = 2,
			levelInterval = 1,
			moveFoodTimeout = 100,
			boardWidth = 25,
			boardHeight = 25,
			foodColor = 'yellow',
			snakeColor = 'green',
			snakeInitialX = 0,
			snakeInitialY = 0,


			direction = {
				DOWN: 1,
				LEFT: 2,
				RIGHT: 3,
				UP: 4
			},


	// GAME STATE		
			paused = false,
			currentRate,
			interval,
			snakeHead = new Point(),
			snakeTail = [],
			tailLength,
			currentDirection,
			nextDirection,
			score,
			food = new Point();
		

	function Point (x, y) {
		return {x: +x, y: +y};
	}

	function pointInList (point, list) {
		for (var i = list.length - 1; i >= 0; i--) {
			if (pointsAreEqual(point, list[i])) {
				return true;
			}
		}
	
		return false;
	}

	function pointsAreEqual (a, b) {
		return a.x == b.x && a.y == b.y;
	}

	function initializeGame () {
		moveFood();
		score = 0;
		currentDirection = direction.RIGHT;
		tailLength = 0;
	
		snakeHead.x = snakeInitialX;
		snakeHead.y = snakeInitialY;
	
		snakeTail = [];
	
		currentRate = beginRate;
		interval = 1000 / currentRate;
	}

	function frame () {
		// Bad practice: should use window.requestAnimationFrame
		// but it would be a bit more complex in this situation
		// and I'm lazy.
		window.setTimeout(frame, interval);

		if (paused) {
			return;
		}
	
		changeDirection();
		moveSnake();
	
		if (snakeTouchingSnake()) {
			endGame();
		}
	
		if (snakeTouchingFood()) {
			earnPoint();
			moveFood();
		}
	
		updateDisplay();
	}

	function pause () {
		paused = true;
	}

	function unpause () {
		paused = false;
	}

	function changeDirection () {
		if (nextDirection == direction.LEFT && currentDirection != direction.RIGHT) {
			currentDirection = direction.LEFT;
		}
	
		if (nextDirection == direction.RIGHT && currentDirection != direction.LEFT) {
			currentDirection = direction.RIGHT;
		}
	
		if (nextDirection == direction.UP && currentDirection != direction.DOWN) {
			currentDirection = direction.UP;
		}
	
		if (nextDirection == direction.DOWN && currentDirection != direction.UP) {
			currentDirection = direction.DOWN;
		}
	}

	function moveSnake () {
		moveSnakeTail();
		moveSnakeHead();
	}

	function moveSnakeHead () {
		snakeHead = alterPointByDirection(snakeHead);
	}

	function moveSnakeTail () {
		if (!tailLength) {
			return;
		}
	
		snakeTail.unshift(new Point(snakeHead.x, snakeHead.y));
	
		while (snakeTail.length > tailLength) {
			snakeTail.pop();
		}
	}

	function alterPointByDirection (item) {
		switch (currentDirection) {
			case direction.UP:
				item.y--;
			break;
		
			case direction.DOWN:
				item.y++;
			break;
		
			case direction.LEFT:
				item.x--;
			break;
		
			case direction.RIGHT:
				item.x++;
			break;
		}
	
		if (item.y > (boardHeight - 1)) {
			item.y = 0;
		}
	
		if (item.y < 0) {
			item.y = boardHeight - 1;
		}
	
	
		if (item.x > (boardWidth - 1)) {
			item.x = 0;
		}
	
		if (item.x < 0) {
			item.x = boardWidth - 1;
		}

		return item;
	}

	function snakeTouchingSnake () {
		return pointInList(snakeHead, snakeTail);
	}

	function snakeTouchingFood () {
		return pointsAreEqual(snakeHead, food);
	}

	function moveFood () {
		window.setTimeout(function () {
			var pointInSnake = false;
		
			do {
				food.x = Math.round(Math.random() * (boardWidth -1));
				food.y = Math.round(Math.random() * (boardHeight - 1));
			
				if (pointInList(food, snakeTail)) {
					pointInSnake = true;
				} else if (pointsAreEqual(food, snakeHead)) {
					pointInSnake = true;
				} else {
					pointInSnake = false;
				}
			} while (pointInSnake);
		
			updateDisplay();
		}, moveFoodTimeout)
	}

	function earnPoint () {
		score++;
	
		if (score % levelInterval == 0) {
			currentRate += .5;
			interval = 1000 / currentRate;
			tailLength++;
		}
	}

	function endGame () {
		pause();
	
		runEndGameDisplayRoutine(function () {	
			initializeGame();
			unpause();
		});
	}


	function initializeDisplay () {
	
		$board = $('#board');
	
		if (!$board.length) {
			$board = $('<div />').attr('id', 'board');
			$('body').append($board);
		}
	
		for (var i = 0; i < boardHeight; i++) {
			$row = $('<div />').addClass('row').addClass('row_' + i);
		
			for (var j = 0; j < boardWidth; j++) {
				id = 'pixel_' + j + '_' + i;
				$pixel = $('<div />').addClass('pixel').addClass('column_' + j).attr('id', id);
				$row.append($pixel);
			}
		
			$board.append($row);
		}
	
		$scoreboard = $('#scoreboard');
	
		if (!$scoreboard.length) {
			$scoreboard = $('<div />').attr('id', 'scoreboard');
			$('body').append($scoreboard);
		}
	
		$scoreboard.text('0');
	}

	function clearDisplay () {
		$('.pixel').removeClass('food').removeClass('snake').removeClass('end').addClass('off');
	}

	function drawPixel (x, y, color) {
		$pixel = $('#pixel_' + x + '_' + y);
		$pixel.removeClass('food').removeClass('snake').removeClass('end').removeClass('off').addClass(color);
	}

	function drawRow (y, color) {
		for (var i = 0; i < boardWidth; i++) {
			drawPixel(i, y, color);
		}
	}

	function drawAll (color) {
		for (var i = 0; i < boardHeight; i++) {
			drawRow(i, color);
		}
	}

	function blink (color, rate, num, callback) {
		on = false;
		clearDisplay();
	
		var i = window.setInterval(function () {
			if (num == 0) {
				clearDisplay();
				window.clearInterval(i);
				callback();
				return;
			}
		
			if (on === false) {
				drawAll(color);
				num--;
				on = true;
			} else {
				on = false;
				clearDisplay();
			}
		}, rate);
	}

	function updateDisplay () {
		clearDisplay();
	
		drawFood();
		drawSnake();
	
		drawScore();
	}

	function drawFood () {
		drawPixel(food.x, food.y, 'food');
	}

	function drawSnake () {
		drawPixel(snakeHead.x, snakeHead.y, 'snake');
	
		if (snakeTail) {
			for (var i = snakeTail.length - 1; i >= 0; i--) {
				drawPixel(snakeTail[i].x, snakeTail[i].y, 'snake');
			}
		}
	}

	function drawScore () {
		var scoreStr = '' + score;
		$('#scoreboard').text(scoreStr);
	}

	// This is really really dumb. 	
	// There would be many better ways to do this.
	function runEndGameDisplayRoutine (callback) {
		for (i = 0; i < boardHeight; i++) {
			window.setTimeout(function (i) {
				drawRow(i, 'end');
			}, i * 100, i);	
		}
	
		window.setTimeout(function () {
			blink('end', 1000, 2, callback)
		}, (i + 1) * 100);
	
	}


	function initializeKeyboard () {
		var table = {
			38: direction.UP,
			40: direction.DOWN,
			39: direction.RIGHT,
			37: direction.LEFT
		}
	
		$(document).on('keydown', function (e) {
		
			var key = e.which;
				
			if (table[key]) {
				e.preventDefault();
				nextDirection = table[key];
			}
		
		});
	}

	$(function () {
		initializeDisplay();
		initializeKeyboard();
		initializeGame();
		frame();
	});
}());