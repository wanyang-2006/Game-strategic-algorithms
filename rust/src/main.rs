use std::cmp::min;
use std::io::{self, Read};
use std::time::{Duration, Instant, SystemTime, UNIX_EPOCH};

const BOARD_WIDTH: i32 = 9;
const BOARD_HEIGHT: i32 = 10;
const MATE_SCORE: i32 = 1_000_000;
const MIN_SCORE: i32 = -999_999;
const MAX_SCORE: i32 = 999_999;

const DX_OB: [i32; 4] = [-1, 1, -1, 1];
const DY_OB: [i32; 4] = [-1, -1, 1, 1];
const DX_STRAI: [i32; 4] = [-1, 0, 0, 1];
const DY_STRAI: [i32; 4] = [0, -1, 1, 0];
const DX_LR: [i32; 2] = [-1, 1];
const DX_KNIGHT: [i32; 8] = [-2, -2, -1, -1, 1, 1, 2, 2];
const DY_KNIGHT: [i32; 8] = [-1, 1, -2, 2, -2, 2, -1, 1];
const DX_KNIGHT_FOOT: [i32; 8] = [-1, -1, 0, 0, 0, 0, 1, 1];
const DY_KNIGHT_FOOT: [i32; 8] = [0, 0, -1, 1, -1, 1, 0, 0];
const DX_BISHOP: [i32; 4] = [-2, -2, 2, 2];
const DY_BISHOP: [i32; 4] = [-2, 2, -2, 2];
const DX_BISHOP_EYE: [i32; 4] = [-1, -1, 1, 1];
const DY_BISHOP_EYE: [i32; 4] = [-1, 1, -1, 1];

#[derive(Clone, Copy, Debug, PartialEq, Eq)]
enum Stone {
    None = 0,
    King = 1,
    Bishop = 2,
    Knight = 3,
    Rook = 4,
    Pawn = 5,
    Cannon = 6,
    Assistant = 7,
}

#[derive(Clone, Copy, Debug, PartialEq, Eq)]
enum Color {
    Black = 0,
    Red = 1,
    Empty = 2,
}

#[derive(Clone, Copy, Debug, PartialEq, Eq)]
struct Grid {
    stone: Stone,
    color: Color,
}

impl Default for Grid {
    fn default() -> Self {
        Self {
            stone: Stone::None,
            color: Color::Empty,
        }
    }
}

#[derive(Clone, Copy, Debug, PartialEq, Eq)]
struct Move {
    source_x: i32,
    source_y: i32,
    target_x: i32,
    target_y: i32,
}

impl Move {
    const fn invalid() -> Self {
        Self {
            source_x: -1,
            source_y: -1,
            target_x: -1,
            target_y: -1,
        }
    }

    fn from_botzone(source: &str, target: &str) -> Option<Self> {
        let s = parse_coord(source)?;
        let t = parse_coord(target)?;
        Some(Self {
            source_x: s.0,
            source_y: s.1,
            target_x: t.0,
            target_y: t.1,
        })
    }

    fn to_json(self) -> String {
        if self.source_x < 0 {
            "{\"response\":{\"source\":\"-1\",\"target\":\"-1\"}}".to_string()
        } else {
            format!(
                "{{\"response\":{{\"source\":\"{}{}\",\"target\":\"{}{}\"}}}}",
                pgn_int_to_char(self.source_x),
                self.source_y,
                pgn_int_to_char(self.target_x),
                self.target_y
            )
        }
    }
}

impl Default for Move {
    fn default() -> Self {
        Self::invalid()
    }
}

#[derive(Clone, Debug)]
struct MoveNode {
    mv: Move,
    value: i32,
}

#[derive(Clone, Copy, Debug, PartialEq, Eq)]
enum HashFlag {
    Exact,
    Alpha,
    Beta,
}

#[derive(Clone, Copy, Debug)]
struct TTStateNode {
    zobrist_key: u64,
    depth: i32,
    score: i32,
    flag: HashFlag,
    best_move: Move,
}

impl Default for TTStateNode {
    fn default() -> Self {
        Self {
            zobrist_key: 0,
            depth: -1,
            score: 0,
            flag: HashFlag::Alpha,
            best_move: Move::invalid(),
        }
    }
}

const TT_SIZE: usize = 1 << 20;
const MAX_DEPTH: usize = 64;
const PHASE_TOTAL: i32 = 66;
const ROOK_STATE: i32 = 6;
const HORSE_CANNON_STATE: i32 = 3;
const OTHER_STATE: i32 = 1;

const VICTIM_VALUE: [i32; 8] = [0, 6, 1, 4, 5, 2, 3, 1];
const ATTACKER_VALUE: [i32; 8] = [0, 6, 1, 4, 5, 2, 3, 1];
const LIVE_VALUE: [i32; 8] = [0, 0, 200, 430, 900, 110, 450, 200];

const MID_HORSE: [i32; 90] = [
    90, 90, 90, 96, 90, 96, 90, 90, 90, 90, 96, 103, 97, 94, 97, 103, 96, 90, 92, 98, 99, 103, 99,
    103, 99, 98, 92, 93, 108, 100, 107, 100, 107, 100, 108, 93, 90, 100, 99, 103, 104, 103, 99,
    100, 90, 90, 98, 101, 102, 103, 102, 101, 98, 90, 92, 94, 98, 95, 98, 95, 98, 94, 92, 93, 92,
    94, 95, 92, 95, 94, 92, 93, 85, 90, 92, 93, 78, 93, 92, 90, 85, 88, 85, 90, 88, 90, 88, 90, 85,
    88,
];
const END_HORSE: [i32; 90] = [
    92, 94, 96, 96, 96, 96, 96, 94, 92, 94, 96, 98, 98, 98, 98, 98, 96, 94, 96, 98, 100, 100, 100,
    100, 100, 98, 96, 96, 98, 100, 100, 100, 100, 100, 98, 96, 96, 98, 100, 100, 100, 100, 100, 98,
    96, 94, 96, 98, 98, 98, 98, 98, 96, 94, 94, 96, 98, 98, 98, 98, 98, 96, 94, 92, 94, 96, 96, 96,
    96, 96, 94, 92, 90, 92, 94, 92, 92, 92, 94, 92, 90, 88, 90, 92, 90, 90, 90, 92, 90, 88,
];
const MID_ROOK: [i32; 90] = [
    206, 208, 207, 213, 214, 213, 207, 208, 206, 206, 212, 209, 216, 233, 216, 209, 212, 206, 206,
    208, 207, 214, 216, 214, 207, 208, 206, 206, 213, 213, 216, 216, 216, 213, 213, 206, 208, 211,
    211, 214, 215, 214, 211, 211, 208, 208, 212, 212, 214, 215, 214, 212, 212, 208, 204, 209, 204,
    212, 214, 212, 204, 209, 204, 198, 208, 204, 212, 212, 212, 204, 208, 198, 200, 208, 206, 212,
    200, 212, 206, 208, 200, 194, 206, 204, 212, 200, 212, 204, 206, 194,
];
const END_ROOK: [i32; 90] = [
    182, 182, 182, 184, 186, 184, 182, 182, 182, 184, 184, 184, 186, 190, 186, 184, 184, 184, 182,
    182, 182, 184, 186, 184, 182, 182, 182, 180, 180, 180, 182, 184, 182, 180, 180, 180, 180, 180,
    180, 182, 184, 182, 180, 180, 180, 180, 180, 180, 182, 184, 182, 180, 180, 180, 180, 180, 180,
    182, 184, 182, 180, 180, 180, 180, 180, 180, 182, 184, 182, 180, 180, 180, 180, 180, 180, 182,
    184, 182, 180, 180, 180, 180, 180, 180, 182, 184, 182, 180, 180, 180,
];
const MID_CANNON: [i32; 90] = [
    100, 100, 96, 91, 90, 91, 96, 100, 100, 98, 98, 96, 92, 89, 92, 96, 98, 98, 97, 97, 96, 91, 92,
    91, 96, 97, 97, 96, 99, 99, 98, 100, 98, 99, 99, 96, 96, 96, 96, 96, 100, 96, 96, 96, 96, 95,
    96, 99, 96, 100, 96, 99, 96, 95, 96, 96, 96, 96, 96, 96, 96, 96, 96, 97, 96, 100, 99, 101, 99,
    100, 96, 97, 96, 97, 98, 98, 98, 98, 98, 97, 96, 96, 96, 97, 99, 99, 99, 97, 96, 96,
];
const END_CANNON: [i32; 90] = [
    100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100,
    100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 102, 104, 102, 100, 100, 100, 100, 100,
    100, 102, 104, 102, 100, 100, 100, 100, 100, 100, 102, 104, 102, 100, 100, 100, 100, 100, 100,
    102, 104, 102, 100, 100, 100, 100, 100, 100, 102, 104, 102, 100, 100, 100, 100, 100, 100, 104,
    106, 104, 100, 100, 100, 100, 100, 100, 104, 106, 104, 100, 100, 100,
];
const KING_SAFE_PTABLE: [i32; 90] = [
    0, 0, 0, -120, -110, -120, 0, 0, 0, 0, 0, 0, -110, -100, -110, 0, 0, 0, 0, 0, 0, -100, -90,
    -100, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, -75, -65, -75, 0, 0, 0, 0, 0, 0, -35, -28, -35, 0, 0, 0, 0,
    0, 0, 12, 45, 12, 0, 0, 0,
];
const ADVISOR_PTABLE: [i32; 90] = [
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 16, 0, 16, 0, 0, 0, 0, 0, 0, 0, 30, 0, 0, 0, 0, 0, 0, 0, 18, 0, 18, 0, 0, 0,
];
const BISHOP_PTABLE: [i32; 90] = [
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 22, 0, 0, 0, 22, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 24, 0, 0, 0, 28, 0, 0, 0, 24, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 18, 0, 0, 0, 18, 0, 0,
];
const PAWN_MID_PTABLE: [i32; 90] = [
    75, 80, 85, 95, 110, 95, 85, 80, 75, 68, 72, 78, 88, 100, 88, 78, 72, 68, 58, 62, 68, 76, 86,
    76, 68, 62, 58, 44, 48, 54, 60, 68, 60, 54, 48, 44, 30, 34, 38, 42, 48, 42, 38, 34, 30, 18, 20,
    24, 28, 32, 28, 24, 20, 18, 8, 10, 12, 16, 20, 16, 12, 10, 8, 0, 0, 2, 4, 8, 4, 2, 0, 0, 0, 0,
    0, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
];
const PAWN_END_PTABLE: [i32; 90] = [
    95, 100, 105, 115, 130, 115, 105, 100, 95, 86, 90, 96, 106, 120, 106, 96, 90, 86, 72, 76, 82,
    92, 104, 92, 82, 76, 72, 56, 60, 66, 74, 84, 74, 66, 60, 56, 38, 42, 48, 54, 62, 54, 48, 42,
    38, 22, 25, 28, 34, 40, 34, 28, 25, 22, 10, 12, 14, 18, 24, 18, 14, 12, 10, 0, 0, 3, 6, 10, 6,
    3, 0, 0, 0, 0, 0, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
];
const HOLLOW_THREAT: [i32; 10] = [80, 80, 80, 70, 60, 50, 40, 30, 20, 10];
const CENTRAL_THREAT: [i32; 10] = [100, 100, 90, 80, 70, 60, 50, 40, 30, 20];
const BELOW_PAWN: i32 = 50;

#[derive(Clone)]
struct Chess {
    state: Vec<Vec<Grid>>,
    curr_turn_id: usize,
    curr_color: Color,
    continue_peace_turn: Vec<i32>,
    last_move_eaten: Vec<bool>,
}

impl Chess {
    fn new() -> Self {
        let mut chess = Self {
            state: Vec::new(),
            curr_turn_id: 0,
            curr_color: Color::Red,
            continue_peace_turn: vec![0],
            last_move_eaten: vec![false],
        };
        chess.reset_board();
        chess
    }

    fn reset_board(&mut self) {
        let mut board = vec![Grid::default(); 90];
        let mut set = |x, y, stone, color| board[Self::xy2pos(x, y)] = Grid { stone, color };

        set(0, 0, Stone::Rook, Color::Red);
        set(1, 0, Stone::Knight, Color::Red);
        set(2, 0, Stone::Bishop, Color::Red);
        set(3, 0, Stone::Assistant, Color::Red);
        set(4, 0, Stone::King, Color::Red);
        set(5, 0, Stone::Assistant, Color::Red);
        set(6, 0, Stone::Bishop, Color::Red);
        set(7, 0, Stone::Knight, Color::Red);
        set(8, 0, Stone::Rook, Color::Red);
        set(1, 2, Stone::Cannon, Color::Red);
        set(7, 2, Stone::Cannon, Color::Red);
        for x in [0, 2, 4, 6, 8] {
            set(x, 3, Stone::Pawn, Color::Red);
        }

        set(0, 9, Stone::Rook, Color::Black);
        set(1, 9, Stone::Knight, Color::Black);
        set(2, 9, Stone::Bishop, Color::Black);
        set(3, 9, Stone::Assistant, Color::Black);
        set(4, 9, Stone::King, Color::Black);
        set(5, 9, Stone::Assistant, Color::Black);
        set(6, 9, Stone::Bishop, Color::Black);
        set(7, 9, Stone::Knight, Color::Black);
        set(8, 9, Stone::Rook, Color::Black);
        set(1, 7, Stone::Cannon, Color::Black);
        set(7, 7, Stone::Cannon, Color::Black);
        for x in [0, 2, 4, 6, 8] {
            set(x, 6, Stone::Pawn, Color::Black);
        }

        self.state.clear();
        self.state.push(board);
        self.curr_turn_id = 0;
        self.curr_color = Color::Red;
        self.continue_peace_turn = vec![0];
        self.last_move_eaten = vec![false];
    }

    fn board(&self) -> &[Grid] {
        &self.state[self.curr_turn_id]
    }

    fn in_board(x: i32, y: i32) -> bool {
        (0..BOARD_WIDTH).contains(&x) && (0..BOARD_HEIGHT).contains(&y)
    }

    fn xy2pos(x: i32, y: i32) -> usize {
        (y * BOARD_WIDTH + x) as usize
    }

    fn pos2xy(pos: usize) -> (i32, i32) {
        ((pos as i32) % BOARD_WIDTH, (pos as i32) / BOARD_WIDTH)
    }

    fn in_king_area(x: i32, y: i32, color: Color) -> bool {
        match color {
            Color::Red => (3..=5).contains(&x) && (0..=2).contains(&y),
            Color::Black => (3..=5).contains(&x) && (7..=9).contains(&y),
            Color::Empty => false,
        }
    }

    fn in_color_area(_x: i32, y: i32, color: Color) -> bool {
        match color {
            Color::Red => y <= 4,
            Color::Black => y >= 5,
            Color::Empty => false,
        }
    }

    fn opp_color(color: Color) -> Color {
        match color {
            Color::Red => Color::Black,
            Color::Black => Color::Red,
            Color::Empty => Color::Empty,
        }
    }

    fn opp(&self) -> Color {
        Self::opp_color(self.curr_color)
    }

    fn grid(&self, x: i32, y: i32) -> Grid {
        self.board()[Self::xy2pos(x, y)]
    }

    fn make_move_assume_legal(&mut self, mv: Move) -> bool {
        if !Self::in_board(mv.source_x, mv.source_y) || !Self::in_board(mv.target_x, mv.target_y) {
            return false;
        }
        let source_pos = Self::xy2pos(mv.source_x, mv.source_y);
        let target_pos = Self::xy2pos(mv.target_x, mv.target_y);
        let source = self.board()[source_pos];
        let target = self.board()[target_pos];
        if source.color != self.curr_color
            || source.stone == Stone::None
            || target.color == self.curr_color
        {
            return false;
        }

        let mut next_state = self.board().to_vec();
        let mut next_peace = self.continue_peace_turn[self.curr_turn_id] + 1;
        if target.color == self.opp() {
            next_peace = 0;
            self.last_move_eaten.push(true);
        } else {
            self.last_move_eaten.push(false);
        }
        next_state[target_pos] = source;
        next_state[source_pos] = Grid::default();
        self.state.push(next_state);
        self.curr_turn_id += 1;
        self.continue_peace_turn.push(next_peace);
        self.curr_color = self.opp();
        true
    }

    fn undo_move(&mut self) {
        if self.curr_turn_id == 0 {
            return;
        }
        self.state.pop();
        self.curr_turn_id -= 1;
        self.curr_color = self.opp();
        self.continue_peace_turn.pop();
        self.last_move_eaten.pop();
    }

    fn make_null_move(&mut self) {
        self.state.push(self.board().to_vec());
        self.curr_turn_id += 1;
        self.continue_peace_turn
            .push(self.continue_peace_turn[self.curr_turn_id - 1] + 1);
        self.last_move_eaten.push(false);
        self.curr_color = self.opp();
    }

    fn undo_null_move(&mut self) {
        self.undo_move();
    }

    fn between_is_empty(&self, sx: i32, sy: i32, ex: i32, ey: i32) -> bool {
        self.between_not_empty_num(sx, sy, ex, ey) == 0
    }

    fn between_not_empty_num(&self, sx: i32, sy: i32, ex: i32, ey: i32) -> i32 {
        let dx = (ex - sx).signum();
        let dy = (ey - sy).signum();
        let (mut x, mut y) = (sx + dx, sy + dy);
        let mut count = 0;
        while Self::in_board(x, y) && !(x == ex && y == ey) {
            if self.grid(x, y).color != Color::Empty {
                count += 1;
            }
            x += dx;
            y += dy;
        }
        count
    }

    fn in_same_straight_line(sx: i32, sy: i32, ex: i32, ey: i32) -> bool {
        sx == ex || sy == ey
    }

    fn attacked(&self, color: Color, mx: i32, my: i32) -> bool {
        for x in 0..BOARD_WIDTH {
            for y in 0..BOARD_HEIGHT {
                let cur = self.grid(x, y);
                if cur.color != color {
                    continue;
                }
                let dex = mx - x;
                let dey = my - y;
                match cur.stone {
                    Stone::King | Stone::Rook => {
                        if Self::in_same_straight_line(x, y, mx, my)
                            && self.between_is_empty(x, y, mx, my)
                        {
                            return true;
                        }
                    }
                    Stone::Assistant => {
                        if Self::in_king_area(mx, my, Self::opp_color(color))
                            && dex.abs() == 1
                            && dey.abs() == 1
                        {
                            return true;
                        }
                    }
                    Stone::Knight => {
                        if dex.abs() == 2 && dey.abs() == 1 {
                            let fx = x + dex.signum();
                            if self.grid(fx, y).color == Color::Empty {
                                return true;
                            }
                        } else if dex.abs() == 1 && dey.abs() == 2 {
                            let fy = y + dey.signum();
                            if self.grid(x, fy).color == Color::Empty {
                                return true;
                            }
                        }
                    }
                    Stone::Bishop => {
                        if dex.abs() == 2 && dey.abs() == 2 {
                            let ex = x + dex / 2;
                            let ey = y + dey / 2;
                            if self.grid(ex, ey).color == Color::Empty {
                                return true;
                            }
                        }
                    }
                    Stone::Pawn => {
                        let forward = if cur.color == Color::Red { 1 } else { -1 };
                        let crossed = if cur.color == Color::Red {
                            y > 4
                        } else {
                            y < 5
                        };
                        if dex == 0 && dey == forward {
                            return true;
                        }
                        if crossed && dex.abs() == 1 && dey == 0 {
                            return true;
                        }
                    }
                    Stone::Cannon => {
                        if Self::in_same_straight_line(x, y, mx, my)
                            && self.between_not_empty_num(x, y, mx, my) == 1
                        {
                            return true;
                        }
                    }
                    Stone::None => {}
                }
            }
        }
        false
    }

    #[allow(dead_code)]
    fn is_attacked(&self, color: Color, mx: i32, my: i32) -> bool {
        let target = self.grid(mx, my);
        if target.stone == Stone::King {
            return self.attacked(color, mx, my);
        }
        for x in 0..BOARD_WIDTH {
            for y in 0..BOARD_HEIGHT {
                let cur = self.grid(x, y);
                if cur.color != color {
                    continue;
                }
                let dex = mx - x;
                let dey = my - y;
                match cur.stone {
                    Stone::King => {
                        if dex.abs() + dey.abs() == 1 && Self::in_king_area(mx, my, color) {
                            return true;
                        }
                    }
                    Stone::Assistant => {
                        if Self::in_king_area(mx, my, color) && dex.abs() == 1 && dey.abs() == 1 {
                            return true;
                        }
                    }
                    Stone::Bishop => {
                        if Self::in_color_area(mx, my, color) && dex.abs() == 2 && dey.abs() == 2 {
                            let ex = x + dex / 2;
                            let ey = y + dey / 2;
                            if self.grid(ex, ey).color == Color::Empty {
                                return true;
                            }
                        }
                    }
                    Stone::Knight => {
                        if dex.abs() == 2 && dey.abs() == 1 {
                            let fx = x + dex.signum();
                            if self.grid(fx, y).color == Color::Empty {
                                return true;
                            }
                        } else if dex.abs() == 1 && dey.abs() == 2 {
                            let fy = y + dey.signum();
                            if self.grid(x, fy).color == Color::Empty {
                                return true;
                            }
                        }
                    }
                    Stone::Rook => {
                        if Self::in_same_straight_line(x, y, mx, my)
                            && self.between_is_empty(x, y, mx, my)
                        {
                            return true;
                        }
                    }
                    Stone::Cannon => {
                        if Self::in_same_straight_line(x, y, mx, my)
                            && self.between_not_empty_num(x, y, mx, my) == 1
                        {
                            return true;
                        }
                    }
                    Stone::Pawn => {
                        let forward = if cur.color == Color::Red { 1 } else { -1 };
                        let crossed = if cur.color == Color::Red {
                            y > 4
                        } else {
                            y < 5
                        };
                        if dex == 0 && dey == forward {
                            return true;
                        }
                        if crossed && dex.abs() == 1 && dey == 0 {
                            return true;
                        }
                    }
                    Stone::None => {}
                }
            }
        }
        false
    }

    fn my_king_attacked_after_move(&mut self, mv: Move) -> bool {
        let origin = self.curr_color;
        if !self.make_move_assume_legal(mv) {
            return true;
        }
        let king = self
            .board()
            .iter()
            .enumerate()
            .find(|(_, g)| g.color == origin && g.stone == Stone::King)
            .map(|(pos, _)| Self::pos2xy(pos));
        let attacked = match king {
            Some((x, y)) => self.attacked(Self::opp_color(origin), x, y),
            None => true,
        };
        self.undo_move();
        attacked
    }

    fn generate_moves(&mut self, must_defend: bool) -> Vec<Move> {
        let mut moves = Vec::new();
        for x in 0..BOARD_WIDTH {
            for y in 0..BOARD_HEIGHT {
                let cur = self.grid(x, y);
                if cur.color != self.curr_color {
                    continue;
                }
                match cur.stone {
                    Stone::King => {
                        for dir in 0..4 {
                            self.try_push(
                                &mut moves,
                                x,
                                y,
                                x + DX_STRAI[dir],
                                y + DY_STRAI[dir],
                                must_defend,
                                |tx, ty, c| Chess::in_king_area(tx, ty, c),
                            );
                        }
                    }
                    Stone::Assistant => {
                        for dir in 0..4 {
                            self.try_push(
                                &mut moves,
                                x,
                                y,
                                x + DX_OB[dir],
                                y + DY_OB[dir],
                                must_defend,
                                |tx, ty, c| Chess::in_king_area(tx, ty, c),
                            );
                        }
                    }
                    Stone::Pawn => {
                        let forward = if cur.color == Color::Red { 1 } else { -1 };
                        self.try_push(
                            &mut moves,
                            x,
                            y,
                            x,
                            y + forward,
                            must_defend,
                            |tx, ty, _| Chess::in_board(tx, ty),
                        );
                        let crossed = if cur.color == Color::Red {
                            y > 4
                        } else {
                            y < 5
                        };
                        if crossed {
                            for dx in DX_LR {
                                self.try_push(
                                    &mut moves,
                                    x,
                                    y,
                                    x + dx,
                                    y,
                                    must_defend,
                                    |tx, ty, _| Chess::in_board(tx, ty),
                                );
                            }
                        }
                    }
                    Stone::Knight => {
                        for dir in 0..8 {
                            let tx = x + DX_KNIGHT[dir];
                            let ty = y + DY_KNIGHT[dir];
                            if !Self::in_board(tx, ty) {
                                continue;
                            }
                            let fx = x + DX_KNIGHT_FOOT[dir];
                            let fy = y + DY_KNIGHT_FOOT[dir];
                            if self.grid(fx, fy).color == Color::Empty {
                                self.try_push(
                                    &mut moves,
                                    x,
                                    y,
                                    tx,
                                    ty,
                                    must_defend,
                                    |tx, ty, _| Chess::in_board(tx, ty),
                                );
                            }
                        }
                    }
                    Stone::Bishop => {
                        for dir in 0..4 {
                            let tx = x + DX_BISHOP[dir];
                            let ty = y + DY_BISHOP[dir];
                            let ex = x + DX_BISHOP_EYE[dir];
                            let ey = y + DY_BISHOP_EYE[dir];
                            if Self::in_board(tx, ty)
                                && Self::in_color_area(tx, ty, cur.color)
                                && self.grid(ex, ey).color == Color::Empty
                            {
                                self.try_push(
                                    &mut moves,
                                    x,
                                    y,
                                    tx,
                                    ty,
                                    must_defend,
                                    |tx, ty, c| {
                                        Chess::in_board(tx, ty) && Chess::in_color_area(tx, ty, c)
                                    },
                                );
                            }
                        }
                    }
                    Stone::Rook => {
                        for dir in 0..4 {
                            let (mut tx, mut ty) = (x + DX_STRAI[dir], y + DY_STRAI[dir]);
                            while Self::in_board(tx, ty) {
                                let target = self.grid(tx, ty);
                                if target.color == self.curr_color {
                                    break;
                                }
                                self.push_if_safe(
                                    &mut moves,
                                    Move {
                                        source_x: x,
                                        source_y: y,
                                        target_x: tx,
                                        target_y: ty,
                                    },
                                    must_defend,
                                );
                                if target.color == self.opp() {
                                    break;
                                }
                                tx += DX_STRAI[dir];
                                ty += DY_STRAI[dir];
                            }
                        }
                    }
                    Stone::Cannon => {
                        for dir in 0..4 {
                            let (mut tx, mut ty) = (x + DX_STRAI[dir], y + DY_STRAI[dir]);
                            while Self::in_board(tx, ty) {
                                let between = self.between_not_empty_num(x, y, tx, ty);
                                if between > 1 {
                                    break;
                                }
                                let target = self.grid(tx, ty);
                                if between == 0 {
                                    if target.color == Color::Empty {
                                        self.push_if_safe(
                                            &mut moves,
                                            Move {
                                                source_x: x,
                                                source_y: y,
                                                target_x: tx,
                                                target_y: ty,
                                            },
                                            must_defend,
                                        );
                                    }
                                } else if target.color == self.opp() {
                                    self.push_if_safe(
                                        &mut moves,
                                        Move {
                                            source_x: x,
                                            source_y: y,
                                            target_x: tx,
                                            target_y: ty,
                                        },
                                        must_defend,
                                    );
                                }
                                tx += DX_STRAI[dir];
                                ty += DY_STRAI[dir];
                            }
                        }
                    }
                    Stone::None => {}
                }
            }
        }
        moves
    }

    fn try_push<F>(
        &mut self,
        moves: &mut Vec<Move>,
        sx: i32,
        sy: i32,
        tx: i32,
        ty: i32,
        must_defend: bool,
        valid_area: F,
    ) where
        F: Fn(i32, i32, Color) -> bool,
    {
        if !valid_area(tx, ty, self.curr_color) || !Self::in_board(tx, ty) {
            return;
        }
        if self.grid(tx, ty).color == self.curr_color {
            return;
        }
        self.push_if_safe(
            moves,
            Move {
                source_x: sx,
                source_y: sy,
                target_x: tx,
                target_y: ty,
            },
            must_defend,
        );
    }

    fn push_if_safe(&mut self, moves: &mut Vec<Move>, mv: Move, must_defend: bool) {
        if !must_defend || !self.my_king_attacked_after_move(mv) {
            moves.push(mv);
        }
    }

    fn generate_moves_with_forbidden(&mut self, must_defend: bool) -> Vec<Move> {
        let first = self.generate_moves(must_defend);
        first
            .into_iter()
            .filter(|m| !self.repeat_after_move(*m))
            .collect()
    }

    fn repeat_after_move(&mut self, mv: Move) -> bool {
        if !self.make_move_assume_legal(mv) {
            return false;
        }
        let mut tmp_id = self.curr_turn_id;
        if self.last_move_eaten[tmp_id] {
            self.undo_move();
            return false;
        }
        loop {
            if tmp_id <= 1 || self.last_move_eaten[tmp_id - 2] {
                break;
            }
            tmp_id -= 2;
        }
        let repeat = (tmp_id..=self.curr_turn_id)
            .rev()
            .step_by(2)
            .filter(|&id| self.state[id] == self.state[self.curr_turn_id])
            .count();
        self.undo_move();
        repeat >= 3
    }

    fn is_move_valid_with_forbidden(&mut self, mv: Move) -> bool {
        self.generate_moves_with_forbidden(true).contains(&mv)
    }

    fn zobrist_like_key(&self) -> u64 {
        let mut h = if self.curr_color == Color::Black {
            0x9e37_79b9_7f4a_7c15
        } else {
            0
        };
        for (pos, grid) in self.board().iter().enumerate() {
            if grid.stone == Stone::None || grid.color == Color::Empty {
                continue;
            }
            let mut x = ((grid.stone as u64) << 4) ^ ((grid.color as u64) << 1) ^ pos as u64;
            x = x.wrapping_mul(0xbf58_476d_1ce4_e5b9);
            x ^= x >> 31;
            h ^= x;
        }
        h
    }
}

struct ChessAi {
    board: Chess,
    deadline: Instant,
    killer_moves: [[Move; 2]; MAX_DEPTH],
    hash_table: Vec<TTStateNode>,
    grid_control: [[i32; 90]; 2],
    have_control: [bool; 90],
}

impl ChessAi {
    fn new(board: Chess) -> Self {
        Self {
            board,
            deadline: Instant::now() + Duration::from_millis(800),
            killer_moves: [[Move::invalid(); 2]; MAX_DEPTH],
            hash_table: vec![TTStateNode::default(); TT_SIZE],
            grid_control: [[0; 90]; 2],
            have_control: [false; 90],
        }
    }

    fn select_move(&mut self) -> Option<Move> {
        let mut selected = self.alpha_beta_pvs_select();
        selected.sort_by(|a, b| b.value.cmp(&a.value));
        selected.first().map(|n| n.mv)
    }

    fn alpha_beta_pvs_select(&mut self) -> Vec<MoveNode> {
        const ROOT_MAX_DEPTH: i32 = 10;
        self.deadline = Instant::now() + Duration::from_millis(800);
        self.killer_moves = [[Move::invalid(); 2]; MAX_DEPTH];

        let moves = self.board.generate_moves_with_forbidden(true);
        let mut selected = self.make_sorted_nodes(moves);
        if selected.is_empty() {
            return selected;
        }

        let mut last_completed = selected.clone();
        for depth in 1..=ROOT_MAX_DEPTH {
            if self.timed_out() {
                selected = last_completed;
                break;
            }
            for i in 0..selected.len() {
                if self.timed_out() {
                    selected = last_completed;
                    break;
                }
                let mv = selected[i].mv;
                if !self.board.make_move_assume_legal(mv) {
                    selected[i].value = MIN_SCORE;
                    continue;
                }
                let score = -self.alpha_beta_pvs_search(depth - 1, -MAX_SCORE, -MIN_SCORE);
                self.board.undo_move();
                selected[i].value = score;
            }
            selected.sort_by(|a, b| b.value.cmp(&a.value));
            last_completed = selected.clone();
            if selected[0].value > 900_000 {
                break;
            }
        }
        selected
    }

    fn alpha_beta_pvs_search(&mut self, depth: i32, mut alpha: i32, beta: i32) -> i32 {
        if self.timed_out() {
            return self.cal_state(self.board.curr_color);
        }
        if self.temporary_lose() {
            return -MATE_SCORE - depth;
        }

        let cur_key = self.board.zobrist_like_key();
        let original_alpha = alpha;
        let (hit, tt_val, best_tt_move) = self.probe_hash(cur_key, depth, alpha, beta);
        if hit {
            return tt_val;
        }

        if depth == 0 {
            return self.quiescence_search(alpha, beta, 15);
        }
        if self.get_win() {
            return MAX_SCORE;
        }

        const NULL_R: i32 = 2;
        if depth >= NULL_R + 1 && !self.has_threat(self.board.curr_color) {
            self.board.make_null_move();
            let null_val = -self.alpha_beta_pvs_search(depth - 1 - NULL_R, -beta, -beta + 1);
            self.board.undo_null_move();
            if null_val >= beta {
                return beta;
            }
        }

        let moves = self.board.generate_moves_with_forbidden(true);
        if moves.is_empty() {
            return -MATE_SCORE - depth;
        }
        let mut nodes = self.make_sorted_nodes(moves);
        self.promote_ordering(&mut nodes, best_tt_move, depth);

        let mut found_pv = false;
        let mut best_move = Move::invalid();
        let mut best_val = MIN_SCORE;

        for move_index in 0..nodes.len() {
            let mv = nodes[move_index].mv;
            let is_quiet = self.is_quiet_move(mv);
            if !self.board.make_move_assume_legal(mv) {
                continue;
            }

            let mut val;
            if found_pv {
                if depth >= 3 && move_index > 0 && is_quiet {
                    val = -self.alpha_beta_pvs_search(depth - 2, -alpha - 1, -alpha);
                    if val > alpha {
                        val = -self.alpha_beta_pvs_search(depth - 1, -beta, -alpha);
                    }
                } else {
                    val = -self.alpha_beta_pvs_search(depth - 1, -alpha - 1, -alpha);
                }
                if val > alpha && val < beta {
                    val = -self.alpha_beta_pvs_search(depth - 1, -beta, -alpha);
                }
            } else {
                val = -self.alpha_beta_pvs_search(depth - 1, -beta, -alpha);
            }
            self.board.undo_move();

            if val > best_val {
                best_val = val;
                best_move = mv;
            }
            if val >= beta {
                self.record_hash(cur_key, depth, val, HashFlag::Beta, mv);
                if is_quiet {
                    let idx = depth as usize;
                    if idx < MAX_DEPTH && self.killer_moves[idx][0] != mv {
                        self.killer_moves[idx][1] = self.killer_moves[idx][0];
                        self.killer_moves[idx][0] = mv;
                    }
                }
                return best_val;
            }
            if val > alpha {
                alpha = val;
                found_pv = true;
            }
        }

        let flag = if best_val > original_alpha {
            HashFlag::Exact
        } else {
            HashFlag::Alpha
        };
        self.record_hash(cur_key, depth, best_val, flag, best_move);
        best_val
    }

    fn quiescence_search(&mut self, mut alpha: i32, beta: i32, depth: i32) -> i32 {
        if self.timed_out() {
            return self.cal_state(self.board.curr_color);
        }
        if depth <= 0 {
            return self.cal_state(self.board.curr_color);
        }

        let in_check = self.curr_king_in_check();
        let mut best_val = MIN_SCORE;

        if !in_check {
            let stand = self.cal_state(self.board.curr_color);
            best_val = stand;
            if stand >= beta {
                return stand;
            }
            if stand > alpha {
                alpha = stand;
            }
        }

        let moves = self.board.generate_moves_with_forbidden(true);
        if in_check && moves.is_empty() {
            return MIN_SCORE + 1000;
        }

        let curr_state = self.board.board().to_vec();
        let mut qs_nodes = Vec::new();
        for mv in moves {
            if qs_nodes.len() >= 128 {
                break;
            }
            let target = curr_state[Chess::xy2pos(mv.target_x, mv.target_y)].stone;
            let attacker = curr_state[Chess::xy2pos(mv.source_x, mv.source_y)].stone;
            if in_check || target != Stone::None {
                let mut value = attacker_value(attacker);
                if target != Stone::None {
                    let protected =
                        self.board
                            .is_attacked(self.board.opp(), mv.target_x, mv.target_y);
                    if !in_check
                        && protected
                        && victim_value(target) < attacker_value(attacker)
                        && victim_value(target) < 3
                    {
                        continue;
                    }
                    value = mvv_lva_score(attacker, target, protected) + 10_000;
                }
                qs_nodes.push(MoveNode { mv, value });
            }
        }

        qs_nodes.sort_by(|a, b| b.value.cmp(&a.value));
        for node in qs_nodes {
            if !self.board.make_move_assume_legal(node.mv) {
                continue;
            }
            let val = -self.quiescence_search(-beta, -alpha, depth - 1);
            self.board.undo_move();
            if val > best_val {
                best_val = val;
            }
            if val >= beta {
                return beta;
            }
            if val > alpha {
                alpha = val;
            }
        }
        best_val
    }

    fn make_sorted_nodes(&mut self, moves: Vec<Move>) -> Vec<MoveNode> {
        let state = self.board.board().to_vec();
        let color = self.board.curr_color;
        let mut nodes: Vec<_> = moves
            .into_iter()
            .map(|mv| MoveNode {
                value: self.cal_cur_move(mv, &state, color),
                mv,
            })
            .collect();
        nodes.sort_by(|a, b| b.value.cmp(&a.value));
        nodes
    }

    fn promote_ordering(&mut self, nodes: &mut Vec<MoveNode>, best_tt_move: Move, depth: i32) {
        let mut next_idx = 0usize;
        if best_tt_move.source_x != -1 {
            if let Some(i) = nodes.iter().position(|n| n.mv == best_tt_move) {
                nodes.swap(0, i);
                next_idx = 1;
            }
        }
        let idx = depth as usize;
        if idx < MAX_DEPTH {
            for k in 0..2 {
                let killer = self.killer_moves[idx][k];
                if killer.source_x == -1 {
                    continue;
                }
                if let Some(rel) = nodes[next_idx..].iter().position(|n| n.mv == killer) {
                    nodes.swap(next_idx, next_idx + rel);
                    next_idx += 1;
                    if next_idx >= nodes.len() {
                        break;
                    }
                }
            }
        }
    }

    fn probe_hash(&self, key: u64, depth: i32, alpha: i32, beta: i32) -> (bool, i32, Move) {
        let entry = self.hash_table[(key as usize) & (TT_SIZE - 1)];
        if entry.zobrist_key != key {
            return (false, 0, Move::invalid());
        }
        let best_move = entry.best_move;
        if entry.depth >= depth {
            match entry.flag {
                HashFlag::Exact => return (true, entry.score, best_move),
                HashFlag::Alpha if entry.score <= alpha => return (true, alpha, best_move),
                HashFlag::Beta if entry.score >= beta => return (true, beta, best_move),
                _ => {}
            }
        }
        (false, 0, best_move)
    }

    fn record_hash(&mut self, key: u64, depth: i32, score: i32, flag: HashFlag, best_move: Move) {
        let idx = (key as usize) & (TT_SIZE - 1);
        let entry = &mut self.hash_table[idx];
        if entry.zobrist_key != key || depth >= entry.depth {
            *entry = TTStateNode {
                zobrist_key: key,
                depth,
                score,
                flag,
                best_move,
            };
        }
    }

    fn timed_out(&self) -> bool {
        Instant::now() >= self.deadline
    }

    fn get_win(&self) -> bool {
        !self
            .board
            .board()
            .iter()
            .any(|g| g.color == self.board.curr_color && g.stone == Stone::King)
    }

    fn temporary_lose(&self) -> bool {
        self.get_win()
    }

    fn curr_king_in_check(&self) -> bool {
        if let Some((x, y)) = self
            .board
            .board()
            .iter()
            .enumerate()
            .find(|(_, g)| g.color == self.board.curr_color && g.stone == Stone::King)
            .map(|(pos, _)| Chess::pos2xy(pos))
        {
            self.board.attacked(self.board.opp(), x, y)
        } else {
            true
        }
    }

    fn has_threat(&self, color: Color) -> bool {
        for y in (0..BOARD_HEIGHT).rev() {
            for x in 0..BOARD_WIDTH {
                let g = self.board.grid(x, y);
                if g.color == color && self.board.is_attacked(Chess::opp_color(color), x, y) {
                    return true;
                }
            }
        }
        false
    }

    fn has_threat_of_value_stone(&self, color: Color) -> bool {
        for y in (0..BOARD_HEIGHT).rev() {
            for x in 0..BOARD_WIDTH {
                let g = self.board.grid(x, y);
                let important = matches!(g.stone, Stone::Rook | Stone::Knight | Stone::Cannon);
                if g.color == color
                    && important
                    && self.board.is_attacked(Chess::opp_color(color), x, y)
                {
                    return true;
                }
            }
        }
        false
    }

    fn is_quiet_move(&mut self, mv: Move) -> bool {
        if !Chess::in_board(mv.source_x, mv.source_y) || !Chess::in_board(mv.target_x, mv.target_y)
        {
            return false;
        }
        let source = self.board.grid(mv.source_x, mv.source_y);
        let target = self.board.grid(mv.target_x, mv.target_y);
        if source.color != self.board.curr_color || source.stone == Stone::None {
            return false;
        }
        if target.color != Color::Empty && target.color != source.color {
            return false;
        }

        if !self.board.make_move_assume_legal(mv) {
            return false;
        }
        let opponent = self.board.curr_color;
        let mover = self.board.opp();
        let opp_king = self
            .board
            .board()
            .iter()
            .enumerate()
            .find(|(_, g)| g.color == opponent && g.stone == Stone::King)
            .map(|(pos, _)| Chess::pos2xy(pos));
        let gives_check = match opp_king {
            Some((x, y)) => self.board.attacked(mover, x, y),
            None => true,
        };
        let creates_threat = self.has_threat_of_value_stone(opponent);
        self.board.undo_move();
        !gives_check && !creates_threat
    }

    fn cal_cur_move(&self, mv: Move, state: &[Grid], color: Color) -> i32 {
        let target_piece = state[Chess::xy2pos(mv.target_x, mv.target_y)].stone;
        let source_piece = state[Chess::xy2pos(mv.source_x, mv.source_y)].stone;
        if target_piece == Stone::None {
            let advance_bonus = if color == Color::Red {
                9 - mv.target_y
            } else {
                mv.target_y
            };
            return attacker_value(source_piece) + advance_bonus;
        }
        let protected = self
            .board
            .is_attacked(Chess::opp_color(color), mv.target_x, mv.target_y);
        mvv_lva_score(source_piece, target_piece, protected)
    }

    fn get_grid_control(&mut self) {
        self.have_control = [false; 90];
        self.grid_control = [[0; 90]; 2];
        let state = self.board.board().to_vec();

        for y in 0..BOARD_HEIGHT {
            for x in 0..BOARD_WIDTH {
                let cur = state[Chess::xy2pos(x, y)];
                if cur.color == Color::Empty {
                    continue;
                }
                let side = side_index(cur.color);
                match cur.stone {
                    Stone::King => {
                        for dir in 0..4 {
                            let tx = x + DX_STRAI[dir];
                            let ty = y + DY_STRAI[dir];
                            if Chess::in_king_area(tx, ty, cur.color) {
                                self.mark_control(side, x, y, tx, ty, cur.color, &state);
                            }
                        }
                        for step in [-1, 1] {
                            let mut ty = y + step;
                            while Chess::in_board(x, ty) {
                                let t = state[Chess::xy2pos(x, ty)];
                                if t.color != Color::Empty {
                                    if t.stone == Stone::King && t.color != cur.color {
                                        self.mark_control(side, x, y, x, ty, cur.color, &state);
                                    }
                                    break;
                                }
                                ty += step;
                            }
                        }
                    }
                    Stone::Assistant => {
                        for dir in 0..4 {
                            let tx = x + DX_OB[dir];
                            let ty = y + DY_OB[dir];
                            if Chess::in_king_area(tx, ty, cur.color) {
                                self.mark_control(side, x, y, tx, ty, cur.color, &state);
                            }
                        }
                    }
                    Stone::Bishop => {
                        for dir in 0..4 {
                            let tx = x + DX_BISHOP[dir];
                            let ty = y + DY_BISHOP[dir];
                            let ex = x + DX_BISHOP_EYE[dir];
                            let ey = y + DY_BISHOP_EYE[dir];
                            if Chess::in_board(tx, ty)
                                && Chess::in_color_area(tx, ty, cur.color)
                                && state[Chess::xy2pos(ex, ey)].color == Color::Empty
                            {
                                self.mark_control(side, x, y, tx, ty, cur.color, &state);
                            }
                        }
                    }
                    Stone::Knight => {
                        for dir in 0..8 {
                            let tx = x + DX_KNIGHT[dir];
                            let ty = y + DY_KNIGHT[dir];
                            let fx = x + DX_KNIGHT_FOOT[dir];
                            let fy = y + DY_KNIGHT_FOOT[dir];
                            if Chess::in_board(tx, ty)
                                && state[Chess::xy2pos(fx, fy)].color == Color::Empty
                            {
                                self.mark_control(side, x, y, tx, ty, cur.color, &state);
                            }
                        }
                    }
                    Stone::Rook => {
                        for dir in 0..4 {
                            let mut tx = x + DX_STRAI[dir];
                            let mut ty = y + DY_STRAI[dir];
                            while Chess::in_board(tx, ty) {
                                self.mark_control(side, x, y, tx, ty, cur.color, &state);
                                if state[Chess::xy2pos(tx, ty)].color != Color::Empty {
                                    break;
                                }
                                tx += DX_STRAI[dir];
                                ty += DY_STRAI[dir];
                            }
                        }
                    }
                    Stone::Cannon => {
                        for dir in 0..4 {
                            let mut tx = x + DX_STRAI[dir];
                            let mut ty = y + DY_STRAI[dir];
                            let mut has_screen = false;
                            while Chess::in_board(tx, ty) {
                                let t = state[Chess::xy2pos(tx, ty)];
                                if !has_screen {
                                    if t.color == Color::Empty {
                                        self.grid_control[side][Chess::xy2pos(tx, ty)] += 1;
                                    } else {
                                        has_screen = true;
                                    }
                                } else if t.color != Color::Empty {
                                    self.mark_control(side, x, y, tx, ty, cur.color, &state);
                                    break;
                                }
                                tx += DX_STRAI[dir];
                                ty += DY_STRAI[dir];
                            }
                        }
                    }
                    Stone::Pawn => {
                        let forward = if cur.color == Color::Red { 1 } else { -1 };
                        let tx = x;
                        let ty = y + forward;
                        if Chess::in_board(tx, ty) {
                            self.mark_control(side, x, y, tx, ty, cur.color, &state);
                        }
                        let crossed = if cur.color == Color::Red {
                            y > 4
                        } else {
                            y < 5
                        };
                        if crossed {
                            for dx in DX_LR {
                                let tx = x + dx;
                                if Chess::in_board(tx, y) {
                                    self.mark_control(side, x, y, tx, y, cur.color, &state);
                                }
                            }
                        }
                    }
                    Stone::None => {}
                }
            }
        }
    }

    fn mark_control(
        &mut self,
        side: usize,
        sx: i32,
        sy: i32,
        tx: i32,
        ty: i32,
        color: Color,
        state: &[Grid],
    ) {
        let pos = Chess::xy2pos(tx, ty);
        self.grid_control[side][pos] += 1;
        let target = state[pos];
        if target.color == Chess::opp_color(color) && target.stone != Stone::None {
            self.have_control[Chess::xy2pos(sx, sy)] = true;
        }
    }

    fn cal_state(&mut self, color: Color) -> i32 {
        self.get_grid_control();
        let target = self.board.board().to_vec();
        let static_score = self.cal_static_state(&target, color);
        let dynamic_score = self.cal_dynamic_state(&target, color);
        let opp = Chess::opp_color(color);
        let my_shape = self.cal_shape_penalty(&target, color);
        let opp_shape = self.cal_shape_penalty(&target, opp);
        (static_score * 60 + dynamic_score * 90) / 100 - my_shape + opp_shape
    }

    fn cal_static_state(&self, target: &[Grid], color: Color) -> i32 {
        let mut res = 0;
        let weight_mid = get_midgame_weight(target);
        for y in (0..BOARD_HEIGHT).rev() {
            for x in 0..BOARD_WIDTH {
                let pos = Chess::xy2pos(x, y);
                let g = target[pos];
                if g.stone == Stone::None || g.color == Color::Empty {
                    continue;
                }
                let rel = rel_index(x, y, g.color);
                let value = static_piece_value(g.stone, rel, weight_mid, self.have_control[pos]);
                if g.color == color {
                    res += value;
                } else {
                    res -= value;
                }
            }
        }
        res
    }

    fn cal_dynamic_state(&self, target: &[Grid], color: Color) -> i32 {
        let mut side_score = [0, 0];
        for y in 0..BOARD_HEIGHT {
            for x in 0..BOARD_WIDTH {
                let pos = Chess::xy2pos(x, y);
                let tmp = target[pos];
                for side_idx in 0..2 {
                    let ctrl = self.grid_control[side_idx][pos];
                    if ctrl <= 0 {
                        continue;
                    }
                    let side = side_from_index(side_idx);
                    let opp = Chess::opp_color(side);
                    let opp_idx = 1 - side_idx;
                    let capped = min(ctrl, 2);
                    let point_weight = if Chess::in_king_area(x, y, opp) {
                        8
                    } else if !Chess::in_color_area(x, y, side) {
                        5
                    } else if Chess::in_king_area(x, y, side) {
                        3
                    } else {
                        2
                    };
                    side_score[side_idx] += capped * point_weight;

                    if tmp.color == opp && tmp.stone != Stone::None {
                        let mut bonus = threat_bonus(tmp.stone) * capped;
                        if self.grid_control[opp_idx][pos] == 0 && tmp.stone != Stone::King {
                            bonus += hanging_penalty(tmp.stone);
                        }
                        side_score[side_idx] += bonus;
                    }
                    if tmp.color == side && tmp.stone != Stone::None && tmp.stone != Stone::King {
                        side_score[side_idx] += defence_bonus(tmp.stone) * capped;
                        if self.grid_control[opp_idx][pos] > ctrl {
                            side_score[side_idx] -= hanging_penalty(tmp.stone);
                        }
                    }
                }
            }
        }
        let idx = side_index(color);
        side_score[idx] - side_score[1 - idx]
    }

    fn cal_shape_penalty(&self, target: &[Grid], color: Color) -> i32 {
        let mut penalty = 0;
        let opp = Chess::opp_color(color);
        let king_y = if color == Color::Red { 0 } else { 9 };
        let center_y = if color == Color::Red { 1 } else { 8 };
        if target[Chess::xy2pos(4, king_y)].stone != Stone::King
            || target[Chess::xy2pos(4, king_y)].color != color
        {
            return 0;
        }
        let center_pos = Chess::xy2pos(4, center_y);
        if target[center_pos].stone != Stone::Assistant || target[center_pos].color != color {
            for y in 0..BOARD_HEIGHT {
                let pos = Chess::xy2pos(4, y);
                if target[pos].stone == Stone::Cannon && target[pos].color == opp {
                    let block_count = self.board.between_not_empty_num(4, y, 4, king_y);
                    let dist = (y - king_y).abs() as usize;
                    if block_count == 0 {
                        penalty += HOLLOW_THREAT[dist];
                    } else if block_count == 1
                        && target[Chess::xy2pos(4, center_y)].stone == Stone::Knight
                        && target[Chess::xy2pos(4, center_y)].color == color
                    {
                        penalty += CENTRAL_THREAT[dist];
                    }
                }
            }
        }

        let opp_bottom = if color == Color::Red { 9 } else { 0 };
        for x in 0..BOARD_WIDTH {
            let g = target[Chess::xy2pos(x, opp_bottom)];
            if g.stone == Stone::Pawn && g.color == color {
                penalty += BELOW_PAWN;
            }
        }
        penalty
    }
}

fn stone_index(stone: Stone) -> usize {
    stone as usize
}

fn side_index(color: Color) -> usize {
    if color == Color::Red { 0 } else { 1 }
}

fn side_from_index(idx: usize) -> Color {
    if idx == 0 { Color::Red } else { Color::Black }
}

fn rel_index(x: i32, y: i32, color: Color) -> usize {
    let rel_y = if color == Color::Red { 9 - y } else { y };
    Chess::xy2pos(x, rel_y)
}

fn interp(mid_value: i32, end_value: i32, weight_mid: i32) -> i32 {
    let weight_end = PHASE_TOTAL - weight_mid;
    (mid_value * weight_mid + end_value * weight_end) / PHASE_TOTAL
}

fn victim_value(stone: Stone) -> i32 {
    VICTIM_VALUE[stone_index(stone)]
}

fn attacker_value(stone: Stone) -> i32 {
    ATTACKER_VALUE[stone_index(stone)]
}

fn mvv_lva_score(attacker: Stone, victim: Stone, protected: bool) -> i32 {
    if victim == Stone::None {
        return 0;
    }
    let penalty = if protected {
        attacker_value(attacker)
    } else {
        0
    };
    100 + victim_value(victim) * 10 - penalty
}

fn get_midgame_weight(target: &[Grid]) -> i32 {
    let mut phase = 0;
    for g in target {
        if g.stone == Stone::None || g.color == Color::Empty {
            continue;
        }
        phase += match g.stone {
            Stone::Rook => ROOK_STATE,
            Stone::Knight | Stone::Cannon => HORSE_CANNON_STATE,
            _ => OTHER_STATE,
        };
    }
    ((2 * PHASE_TOTAL - phase) * phase / PHASE_TOTAL).clamp(0, PHASE_TOTAL)
}

fn normalized_piece_square(stone: Stone, pos: usize, weight_mid: i32, capture: bool) -> i32 {
    match stone {
        Stone::King => KING_SAFE_PTABLE[pos],
        Stone::Assistant => ADVISOR_PTABLE[pos],
        Stone::Bishop => BISHOP_PTABLE[pos],
        Stone::Pawn => {
            let mut v = interp(PAWN_MID_PTABLE[pos], PAWN_END_PTABLE[pos], weight_mid);
            if capture {
                v += 18;
            }
            v
        }
        Stone::Knight => interp(
            (MID_HORSE[pos] - 95) * 2,
            (END_HORSE[pos] - 96) * 2,
            weight_mid,
        ),
        Stone::Rook => interp(MID_ROOK[pos] - 205, END_ROOK[pos] - 182, weight_mid),
        Stone::Cannon => interp(
            (MID_CANNON[pos] - 98) * 2,
            (END_CANNON[pos] - 101) * 2,
            weight_mid,
        ),
        Stone::None => 0,
    }
}

fn static_piece_value(stone: Stone, pos: usize, weight_mid: i32, capture: bool) -> i32 {
    LIVE_VALUE[stone_index(stone)] + normalized_piece_square(stone, pos, weight_mid, capture)
}

fn threat_bonus(stone: Stone) -> i32 {
    match stone {
        Stone::King => 500,
        Stone::Rook => 135,
        Stone::Cannon => 82,
        Stone::Knight => 78,
        Stone::Assistant | Stone::Bishop => 34,
        Stone::Pawn => 24,
        Stone::None => 0,
    }
}

fn defence_bonus(stone: Stone) -> i32 {
    match stone {
        Stone::Rook => 20,
        Stone::Cannon | Stone::Knight => 14,
        Stone::Assistant | Stone::Bishop => 10,
        Stone::Pawn => 6,
        _ => 0,
    }
}

fn hanging_penalty(stone: Stone) -> i32 {
    if stone == Stone::King || stone == Stone::None {
        return 0;
    }
    (LIVE_VALUE[stone_index(stone)] / 2).clamp(12, 140)
}

fn parse_coord(s: &str) -> Option<(i32, i32)> {
    if s == "-1" || s.len() < 2 {
        return None;
    }
    let mut chars = s.chars();
    let x = chars.next()? as i32 - 'a' as i32;
    let y = chars.next()?.to_digit(10)? as i32;
    Some((x, y))
}

fn pgn_int_to_char(i: i32) -> char {
    char::from_u32(('a' as u32) + i as u32).unwrap_or('a')
}

#[derive(Clone, Debug)]
struct BotMove {
    source: String,
    target: String,
}

fn apply_botzone_input(chess: &mut Chess, input: &str) -> Result<(), String> {
    let requests = extract_move_array(input, "requests")?;
    let responses = extract_move_array(input, "responses")?;
    if requests.len() < responses.len() + 1 {
        return Err("missing current request".to_string());
    }

    for i in 0..responses.len() {
        apply_side_move(chess, &requests[i])?;
        apply_side_move(chess, &responses[i])?;
    }
    apply_side_move(chess, &requests[responses.len()])?;
    Ok(())
}

fn apply_side_move(chess: &mut Chess, item: &BotMove) -> Result<(), String> {
    let source = item.source.as_str();
    let target = item.target.as_str();
    if source == "-1" {
        return Ok(());
    }
    let mv = Move::from_botzone(source, target).ok_or("invalid coordinate")?;
    if !chess.is_move_valid_with_forbidden(mv) {
        return Err("input is not valid".to_string());
    }
    chess.make_move_assume_legal(mv);
    Ok(())
}

fn extract_move_array(input: &str, key: &str) -> Result<Vec<BotMove>, String> {
    let array = extract_array_slice(input, key).ok_or_else(|| format!("missing {key}"))?;
    let mut moves = Vec::new();
    let bytes = array.as_bytes();
    let mut i = 0usize;
    while i < bytes.len() {
        if bytes[i] == b'{' {
            if let Some(end) = find_matching(array, i, b'{', b'}') {
                let obj = &array[i..=end];
                let source =
                    extract_string_field(obj, "source").unwrap_or_else(|| "-1".to_string());
                let target =
                    extract_string_field(obj, "target").unwrap_or_else(|| "-1".to_string());
                moves.push(BotMove { source, target });
                i = end + 1;
                continue;
            }
        }
        i += 1;
    }
    Ok(moves)
}

fn extract_array_slice<'a>(input: &'a str, key: &str) -> Option<&'a str> {
    let needle = format!("\"{}\"", key);
    let key_pos = input.find(&needle)?;
    let after_key = &input[key_pos + needle.len()..];
    let rel_start = after_key.find('[')?;
    let start = key_pos + needle.len() + rel_start;
    let end = find_matching(input, start, b'[', b']')?;
    Some(&input[start + 1..end])
}

fn find_matching(s: &str, start: usize, open: u8, close: u8) -> Option<usize> {
    let bytes = s.as_bytes();
    if bytes.get(start).copied()? != open {
        return None;
    }
    let mut depth = 0i32;
    let mut in_string = false;
    let mut escaped = false;
    for i in start..bytes.len() {
        let b = bytes[i];
        if in_string {
            if escaped {
                escaped = false;
            } else if b == b'\\' {
                escaped = true;
            } else if b == b'"' {
                in_string = false;
            }
            continue;
        }
        if b == b'"' {
            in_string = true;
        } else if b == open {
            depth += 1;
        } else if b == close {
            depth -= 1;
            if depth == 0 {
                return Some(i);
            }
        }
    }
    None
}

fn extract_string_field(obj: &str, key: &str) -> Option<String> {
    let needle = format!("\"{}\"", key);
    let key_pos = obj.find(&needle)?;
    let after_key = &obj[key_pos + needle.len()..];
    let colon = after_key.find(':')?;
    let mut rest = &after_key[colon + 1..];
    rest = rest.trim_start();
    if !rest.starts_with('"') {
        return None;
    }
    parse_json_string(rest)
}

fn parse_json_string(s: &str) -> Option<String> {
    let mut out = String::new();
    let mut chars = s[1..].chars();
    while let Some(ch) = chars.next() {
        match ch {
            '"' => return Some(out),
            '\\' => {
                let escaped = chars.next()?;
                match escaped {
                    '"' => out.push('"'),
                    '\\' => out.push('\\'),
                    '/' => out.push('/'),
                    'b' => out.push('\u{0008}'),
                    'f' => out.push('\u{000c}'),
                    'n' => out.push('\n'),
                    'r' => out.push('\r'),
                    't' => out.push('\t'),
                    'u' => {
                        let mut code = 0u32;
                        for _ in 0..4 {
                            code = code * 16 + chars.next()?.to_digit(16)?;
                        }
                        if let Some(c) = char::from_u32(code) {
                            out.push(c);
                        }
                    }
                    _ => return None,
                }
            }
            _ => out.push(ch),
        }
    }
    None
}

// Opening book is intentionally disabled for the Rust port at the user's request.
// fn get_opening_book(_chess: &mut Chess) -> Option<Move> {
//     None
// }

fn output_fallback(chess: Option<&mut Chess>) -> String {
    if let Some(chess) = chess {
        if let Some(mv) = chess.generate_moves_with_forbidden(true).first().copied() {
            return mv.to_json();
        }
    }
    Move::invalid().to_json()
}

fn give_output_botzone(chess: &mut Chess) -> String {
    // Opening book temporarily disabled.
    // if let Some(mv) = get_opening_book(chess) {
    //     return mv.to_json();
    // }

    let search_board = chess.clone();
    let mut ai = ChessAi::new(search_board);
    if let Some(mv) = ai.select_move() {
        if chess.is_move_valid_with_forbidden(mv) {
            return mv.to_json();
        }
    }

    let moves = chess.generate_moves_with_forbidden(true);
    if moves.is_empty() {
        Move::invalid().to_json()
    } else {
        let idx = pseudo_random_index(moves.len());
        moves.get(idx).copied().unwrap_or(Move::invalid()).to_json()
    }
}

fn pseudo_random_index(len: usize) -> usize {
    if len == 0 {
        return 0;
    }
    let nanos = SystemTime::now()
        .duration_since(UNIX_EPOCH)
        .map(|d| d.subsec_nanos() as usize)
        .unwrap_or(0);
    nanos % len
}

fn main() {
    let mut input = String::new();
    let _ = io::stdin().read_to_string(&mut input);
    let mut chess = Chess::new();
    let result = (|| -> Result<String, String> {
        let raw = input.lines().next().unwrap_or(input.trim());
        apply_botzone_input(&mut chess, raw)?;
        Ok(give_output_botzone(&mut chess))
    })();

    let output = match result {
        Ok(v) => v,
        Err(e) => {
            eprintln!("[FATAL] {e}");
            output_fallback(Some(&mut chess))
        }
    };
    println!("{output}");
}
