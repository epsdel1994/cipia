/*
    This file is part of cipia.
    See [cipia](https://github.com/epsdel1994/cipia) for detail.

    Copyright 2017 T.Hironaka

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifdef EMSCRIPTEN
#include <emscripten.h>
#endif

#include <stdbool.h>

bool cipia_vs[8];

#include "edax-6x6/bit.c"
#include "edax-6x6/board.c"
#include "edax-6x6/move.c"

#include "edax-6x6/stats.c"
#include "edax-6x6/eval.c"
#include "edax-6x6/hash.c"
#include "edax-6x6/ybwc.c"
#include "edax-6x6/search.c"
#include "edax-6x6/endgame.c"
#include "edax-6x6/midgame.c"
#include "edax-6x6/root.c"

#include "edax-6x6/options.c"
#include "edax-6x6/util.c"

#include "edax-6x6/perft.c"
#include "edax-6x6/obftest.c"
#include "edax-6x6/histogram.c"
#include "edax-6x6/bench.c"

#include "edax-6x6/book.c"

#include "edax-6x6/game.c"
#include "edax-6x6/base.c"
#include "edax-6x6/opening.c"

#include "edax-6x6/play.c"
#include "edax-6x6/event.c"
#include "edax-6x6/ui.c"
#include "edax-6x6/edax.c"
#include "edax-6x6/cassio.c"
#include "edax-6x6/ggs.c"
#include "edax-6x6/gtp.c"
#include "edax-6x6/nboard.c"
#include "edax-6x6/xboard.c"

int continue_f;
int continue_f_d;

long long time_limit;

int n_empties_init;
int n_empties_bound;

int n_cells_board;

double progress_res;

unsigned long long board_mask;

HashTable ht[1];
HashData hd[1];

void sighandler(int sig)
{
	continue_f = 0;
	return;
}

char board_str[80];

void make6x6book(int n_empties, Book *book, Board *board, int alpha, int beta, int *upper, int *lower, double progress, double weight, bool initial_flag)
{
	struct timeval tv;
	gettimeofday(&tv, NULL);
	if((tv.tv_sec * 1000 + tv.tv_usec / 1000) > time_limit){
        continue_f = 0;
    };

	if(continue_f == 0){
		*upper = n_cells_board;
		*lower = -n_cells_board;
        if(continue_f_d == 0){
            progress_res = progress;
            continue_f_d = 1;
        }
		return;
	}
	int next_upper, next_lower;
	int prev_upper = n_cells_board, prev_lower = -n_cells_board;

	Move move[1];

    unsigned char move_res = NOMOVE;
    unsigned char move_res_prev = NOMOVE;

	*upper = -n_cells_board;
	*lower = -n_cells_board;

    Position *q = book_probe(book, board);
    if(q){
        move_res_prev = q->leaf.move;
        if( (q->score.upper == q->score.lower) || (q->score.upper <= alpha) || (q->score.lower >= beta) ){
            *upper = q->score.upper;
            *lower = q->score.lower;
            return;
        } else {
            prev_upper = q->score.upper;
            prev_lower = q->score.lower;
            book_remove(book, q);
        }
    } else if( hash_get(ht, board_get_hash_code(board), hd) ) {
        move_res_prev = hd->move[0];
        if( (hd->upper == hd->lower) || (hd->upper <= alpha) || (hd->lower >= beta) ){
            *upper = hd->upper;
            *lower = hd->lower;
            return;
        } else {
            prev_upper = hd->upper;
            prev_lower = hd->lower;
        }
    }

	Position p[1];
	position_init(p);

	p->board->player = board->player;
	p->board->opponent = board->opponent;

	p->level = n_cells_board;

    int book_lower = -n_cells_board;

	unsigned long long x;
	unsigned long long moves = get_moves(board->player, board->opponent) & book->mask;
	if( moves ){
		foreach_bit ( x, moves) {
			board_get_move(board, x, move);
			board_update(board, move);
			Position *r = book_probe(book, board);
			if(r){
				if( book_lower < -r->score.upper ){
                    book_lower = -r->score.upper;
                    move_res = x;
                }
			}
			board_restore(board, move);
		}
	}

    *lower = book_lower;
    *upper = book_lower;

    if(alpha < book_lower){
        alpha = book_lower;
    }

	moves = get_moves(board->player, board->opponent) & book->mask;
	if( moves ){
        if(initial_flag == true){
            unsigned long long move_new = 0llu;
            Board *boards = malloc(sizeof(Board) * bit_count(moves));
            int i=0;
            foreach_bit(x, moves){
                board_get_move(board, x, move);
                board_update(board, move);

                board_unique(board, boards+i);
                bool res = false;
                for(int j=0; j<i; j++){
                    if(board_equal(boards+i, boards+j) == true){
                        res = true;
                    }
                }
                if( res == false){
                    move_new |= (1llu<<x);
                    i++;
                }

                board_restore(board, move);
            }
            free(boards);
            moves = move_new;
        }

        Move *move;
        MoveList movelist[1];
        movelist_get_moves_c(movelist, board, moves);
        int mobility_sum = 0;
        foreach_move(move, movelist){
            board_update(board, move);
            move->score = -get_mobility_c(board->player, board->opponent, book->mask);
            if( move->score == 0 ){ move->score = -1; }
            mobility_sum -= move->score;
            board_restore(board, move);
        }
        foreach_best_move(move, movelist){
			if(alpha >= beta){
				*upper = n_cells_board;
				break;
			}

			board_update(board, move);
			make6x6book(n_empties-1, book, board, -beta, -alpha, &next_upper, &next_lower, progress, -((float)move->score/mobility_sum) *weight, false);
            progress -= ( (double)move->score / mobility_sum ) * weight;
			board_restore(board, move);

			if( alpha < -next_lower ){
                alpha = -next_lower;
            }
			if( *upper < -next_lower ) *upper = -next_lower;
			if( *lower < -next_upper ){
                *lower = -next_upper;
                move_res = move->x;
            }
        }
        
	} else {
		if( can_move_c(board->opponent, board->player, book->mask) ){
			board_pass(board);
			make6x6book(n_empties, book, board, -beta, -alpha, &next_upper, &next_lower, progress, weight, false);
			board_pass(board);

			*upper = -next_lower;
			*lower = -next_upper;
            move_res = PASS;
		} else {
			alpha = board_solve(board, n_empties);

			*upper = alpha;
			*lower = alpha;
			position_free(p);
			return;
		}
	}

	if(*upper > prev_upper) *upper = prev_upper;
	if(*lower < prev_lower){
        *lower = prev_lower;
        move_res = move_res_prev;
    }

	p->score.upper = *upper;
	p->score.lower = *lower;
	p->score.value = (p->score.upper + p->score.lower) / 2;
    p->leaf.move = move_res;
//    p->leaf.move = NOMOVE;

	if(n_empties >= n_empties_bound ){
		position_unique(p);
		book_add(book, p);
	} else {
        hash_feed(ht, board_get_hash_code(board), n_empties, 0, p->score.lower, p->score.upper, p->leaf.move);
		position_free(p);
	}

	return;
}

void update_cipia_vs(unsigned long long mask)
{
    Board board[1], sym[1];
    board->player = mask;
    board->opponent = 0ull;

    for(int i=0; i<8; i++){
        board_symetry(board, i, sym);
        cipia_vs[i] = board_equal(board, sym);
    }
}
/*
int main()
{
	while( (first_flag) || (NULL != fgets(buf, 256, stdin)) ){
        if(first_flag){
            buf[0]='\0';
            first_flag = false;
        }
		if( buf[strlen(buf)-1] == '\n' ){
			buf[strlen(buf)-1] = '\0';
		}
		cur = strtok(buf, " ");
		if(cur == NULL) {
		} else if( !strcmp(cur, "exit") ){
			break;
		} else if( !strcmp(cur, "play") ){
			play_flag = true;
		} else if( !strcmp(cur, "backend") ){
			play_flag = false;
		} else if( !strcmp(cur, "merge") ){
			cur = strtok(NULL, " ");
			if( cur == NULL ){
				fprintf(stderr, "missing filename.\n");
			} else {
				Book src[1];
				book_init(src);
				book_load(src, cur);

                Board src_mask[1], book_mask[1];
                src_mask->player = src->mask; src_mask->opponent = 0llu;
                book_mask->player = book->mask; book_mask->opponent = 0llu;

                for(int i=0; i<8; i++){
                    Board sym[1];
                    board_symetry(src_mask, i, sym);
                    if( board_equal(sym, book_mask) ){
                        Position *p;
                        PositionArray *a;
                        foreach_position(p, a, src){
                            position_symetry(p, i);
                            Position *q = book_probe(book, p->board);
                            if(q){
                                if( q->score.upper > p->score.upper ){
                                    q->score.upper = p->score.upper;
                                }
                                if( q->score.lower < p->score.lower ){
                                    q->score.lower = p->score.lower;
                                }
                            } else {
                                book_add(book, p);
                            }
                        }
                        book_free(src);
                        break;
                    }
                }
			}
		} else if( !strcmp(cur, "new") ){
			int depth;
			cur = strtok(NULL, " ");
			if( cur == NULL ){
				fprintf(stderr, "missing depth. set to 15\n");
                depth = 15;
			} else {
                depth = strtol(cur, NULL, 10);
            }

            unsigned long long old_mask = book->mask;
            book_free(book);
            book_init(book);
            book->options.level = n_cells_board;
            book->options.n_empties = 61 - depth;
            book->options.midgame_error = 0;
            book->options.endcut_error = 0;
            book->mask = old_mask;
            book->P = board->player;
            book->O = board->opponent;
            player = BLACK;
            gamepos = 0;

		} else if( !strcmp(cur, "set") ){
			cur = strtok(NULL, " ");
            if( cur == NULL ){
                fprintf(stderr, "missing board.\n");
            } else {
                unsigned long long mask;
                board_set_c(board, cur, &mask);

                n_cells_board = bit_count(mask);
                update_cipia_vs(mask);

                book_free(book);
                book_init(book);
                book->options.level = n_cells_board;
                book->options.n_empties = 61 - 15;
                book->options.midgame_error = 0;
                book->options.endcut_error = 0;
                book->mask = mask;
                book->P = board->player;
                book->O = board->opponent;
                player = BLACK;
                gamepos = 0;
            }
		} else if( !strcmp(cur, "load") ){
			cur = strtok(NULL, " ");
			if( cur == NULL ){
				fprintf(stderr, "missing filename.\n");
			} else {
				Book *tmp = malloc(sizeof(Book));
				book_init(tmp);
				if( book_load(tmp, cur) ){
					book_free(book);
					free(book);
					book = tmp;

                    board->player = book->P;
                    board->opponent = book->O;
                    gamepos = 0;
                    player = BLACK;

                    n_cells_board = bit_count(book->mask);
                    update_cipia_vs(book->mask);
				} else {
					book_free(tmp);
					free(tmp);
					if( play_flag ) {
						printf("failed to load book file [%s].\n", cur);
					}
				}
			}
		} else if( !strcmp(cur, "save") ){
			cur = strtok(NULL, " ");
			if( cur == NULL ){
				fprintf(stderr, "missing filename.\n");
			} else {
				book_save(book, cur);
			}
		} else if( !strcmp(cur, "move") ){
			cur = strtok(NULL, " ");
			if( cur == NULL ){
				fprintf(stderr, "missing move.\n");
			} else {
				if ( (cur != parse_move(cur, board, game + (gamepos))) && (x_to_bit((game+gamepos)->x) & book->mask) ){
					player ^= 1;
					board_update(board, game + (gamepos++));
				} else {
					fprintf(stderr, "invalid move.\n");
				}
			}
		} else if( !strcmp(cur, "unmove") ){
			if( gamepos != 0 ){
				board_restore(board, game + (--gamepos));
				player ^= 1;
				if( !can_move_c(board->player, board->opponent, book->mask) ){
					board_restore(board, game + (--gamepos));
					player ^= 1;
				}
			} else {
				fprintf(stderr, "cannot back.\n");
			}
		} else if( !strcmp(cur, "eval") ){
		} else {
			fprintf(stderr, "invalid command.\n");
		}

		if( !can_move_c(board->player, board->opponent, book->mask) ){
			if ( can_move_c(board->opponent, board->player, book->mask) ){
				game[gamepos++] = MOVE_PASS;
				board_pass(board);
				player ^= 1;
				board_to_string(board, player, board_str);
				if( play_flag ){
                    printf(" ABCDEFGH\n");
                    for(int i=0; i<8; i++){
                        printf("%d", i+1);
                        for(int j=0; j<8; j++){
                            if ( book->mask & (1llu<<(i*8+j))) {
                                printf("%c", board_str[i*8+j]);
                            } else {
                                printf(" ");
                            }
                        }
                        printf("\n");
                    }
					printf("\n%c's turn.\n\n", board_str[65]);
				} else {
					printf("%s C\n",board_str);
				}
			} else {
				board_to_string(board, player, board_str);
				if( play_flag ){
                    printf(" ABCDEFGH\n");
                    for(int i=0; i<8; i++){
                        printf("%d", i+1);
                        for(int j=0; j<8; j++){
                            if ( book->mask & (1llu<<(i*8+j))) {
                                printf("%c", board_str[i*8+j]);
                            } else {
                                printf(" ");
                            }
                        }
                        printf("\n");
                    }
					printf("\ngame over.\n\n");
				} else {
					printf("%s O\n",board_str);
				}
			}
		} else {
			board_to_string_c(board, player, board_str, book->mask);
			if( play_flag ){
				printf(" ABCDEFGH\n");
                for(int i=0; i<8; i++){
                    printf("%d", i+1);
                    for(int j=0; j<8; j++){
                        if ( book->mask & (1llu<<(i*8+j))) {
                            printf("%c", board_str[i*8+j]);
                        } else {
                            printf(" ");
                        }
                    }
                    printf("\n");
                }
				printf("\n%c's turn.\n\n", board_str[64]);
			} else {
				printf("%s C\n",board_str);
			}
		}

		if( moves ) {
			if( board_count_empties(board)-(64-n_cells_board) > book->options.n_empties-(64-n_cells_board)+3){
			} else if( auto_eval == true ){
				continue_f=1;
                double progress = 0;
                double weight = 1.0f / get_mobility_c(board->player, board->opponent, book->mask);
				foreach_bit(x, moves){
#ifdef EMSCRIPTEN
    EM_ASM_DOUBLE({
self.postMessage({ "kind": "evaling", "progress": $0, });
    }, progress);
#else
#endif
					int upper, lower;
					board_get_move(board, x, move);
					board_update(board, move);

					n_empties_init = board_count_empties(board)-(64-n_cells_board);
					n_empties_bound = book->options.n_empties-(64-n_cells_board)+3;

					make6x6book(n_empties_init, book, board, -n_cells_board, n_cells_board, &upper, &lower, progress, weight, false);
					board_restore(board, move);

					char tmp2[2];
					move_to_string(x, BLACK, tmp2);
					printf("%s %d %d\n", tmp2, -upper, -lower);
                    progress += weight;
				}
			}
		}
		auto_eval = false;

		printf("endl\n");
		fflush(stdout);
	}

	return 0;
}
*/

Book *book;
Board board[1];
int gamepos = 0;
Move game[80];
Move move[1];

bool play_flag = true;
int player = BLACK;
char buf[65535], *cur;
bool first_flag = true;

double c_ems_eval(const char* boardstr, double time)
{
	struct timeval tv;
	gettimeofday(&tv, NULL);
	time_limit = (tv.tv_sec * 1000 + tv.tv_usec / 1000) + 500;

    unsigned long long mask;
    board_set_c(board, boardstr, &mask);
    if(boardstr[65] == 'O'){
        board_swap_players(board);
    }
    if(mask != book->mask){ return -1; }

    unsigned long long  x;
    unsigned long long moves = get_moves(board->player, board->opponent) & book->mask;

    double progress = 0;
    double weight = 1;
    int alpha, beta, upper, lower;
    alpha = -n_cells_board; beta = n_cells_board;
    continue_f = 1;
    continue_f_d = 0;

    n_empties_init = board_count_empties(board) - (64-n_cells_board);
    n_empties_bound = book->options.n_empties - (64-n_cells_board) + 3;

    hash_clear(ht);
    make6x6book(n_empties_init, book, board, alpha, beta, &upper, &lower, progress, weight, true);

    if(continue_f_d == 1){
        return progress_res;
    } else {
        return -1;
    }
}

int c_ems_cache(const char* boardstr)
{
    unsigned long long mask;
    board_set_c(board, boardstr, &mask);
    if(boardstr[65] == 'O'){
        board_swap_players(board);
    }

    unsigned long long  x;
    unsigned long long moves = get_moves(board->player, board->opponent) & book->mask;

    int resx, resy, max_lower, max_upper;

    if(bit_count(moves) == 1){
        foreach_bit(x, moves){
            char tmp2[2];
            move_to_string(x, BLACK, tmp2);
            resx= tmp2[0]-'A'; resy= tmp2[1]-'1';
        }
    } else {
        resx=-1, resy=-1;
        max_upper = -n_cells_board, max_lower = -n_cells_board;
        int res;
        foreach_bit(x, moves){
            board_get_move(board, x, move);
            board_update(board, move);
            Position *q = book_probe(book, board);
            char tmp2[2];
            move_to_string(x, BLACK, tmp2);
            if(q){
                if((max_lower < -q->score.upper) || ( (max_lower == -q->score.upper) && (max_upper < -q->score.lower))){
                    resx = tmp2[0]-'A'; resy = tmp2[1]-'1';
                    max_lower = -q->score.upper; max_upper = -q->score.lower;
                }
            } else if(hash_get(ht, board_get_hash_code(board), hd)){
                if((max_lower < -hd->upper) || ( (max_lower == -hd->upper) && (max_upper < -hd->lower))){
                    resx = tmp2[0]-'A'; resy = tmp2[1]-'1';
                    max_lower = -hd->upper; max_upper = -hd->lower;
                }
            }
            board_restore(board, move);
        }

        Position *q = book_probe(book, board);
        char tmp2[2];
        if(q){
            int move_sym=0;
            int i;
            for(i=0; i<8; i++){
                Board sym[1];
                board_symetry(q->board, i, sym);
                if(board_equal(board, sym) == true){
                    move_sym = symetry(q->leaf.move, i);
                    break;
                }
            }
            move_to_string(move_sym, BLACK, tmp2);
            if(move_sym != NOMOVE){
                resx = tmp2[0]-'A'; resy = tmp2[1]-'1';
            }
        } else if(hash_get(ht, board_get_hash_code(board), hd)){
            move_to_string(hd->move[0], BLACK, tmp2);
            if(hd->move[0] != NOMOVE){
                resx = tmp2[0]-'A'; resy = tmp2[1]-'1';
            }
        }
    }

#ifdef EMSCRIPTEN
    if( board_count_empties(board) - (64-n_cells_board) >= n_empties_bound ){
        Position *q = book_probe(book, board);
        if(q){
            EM_ASM_INT({
                self.postMessage({ kind: 'searched', x: $0, y: $1, upper: $2, lower: $3, });
            }, resx, resy, q->score.upper, q->score.lower);
        } else {
            EM_ASM_INT({
                self.postMessage({ kind: 'searched', x: $0, y: $1, upper: $2, lower: $3, });
            }, resx, resy, n_cells_board, -n_cells_board);
        }
    } else {
        if( hash_get(ht,board_get_hash_code(board), hd) ){
            EM_ASM_INT({
                self.postMessage({ kind: 'searched', x: $0, y: $1, upper: $2, lower: $3, });
            }, resx, resy, hd->upper, hd->lower);
        } else {
            EM_ASM_INT({
                self.postMessage({ kind: 'searched', x: $0, y: $1, upper: $2, lower: $3, });
            }, resx, resy, n_cells_board, -n_cells_board);
        }
    }
#endif
    return 0;
}

int c_ems_load(const char* boardstr)
{
	hash_code_init();
	hash_move_init();

    unsigned long long mask;

	board_init(board);

    board_set_c(board, boardstr, &mask);
    if(boardstr[65] == 'O'){
        board_swap_players(board);
    }

    n_cells_board = bit_count(mask);
    update_cipia_vs(mask);

	book = malloc(sizeof(Book));
    book_init(book);
    book->options.level = n_cells_board;
    book->options.n_empties = 61 - 15;
    book->options.midgame_error = 0;
    book->options.endcut_error = 0;
    book->P = board->player;
    book->O = board->opponent;
    book->mask = mask;
    player = BLACK;
    gamepos = 0;

    hash_init(ht, 1u<<20);

    return 0;
}

int c_ems_book()
{
	hash_code_init();
	hash_move_init();

    unsigned long long mask = 0;

	board_init(board);

//    board_set_c(board, boardstr, &mask);
//    if(boardstr[65] == 'O'){
//        board_swap_players(board);
//    }

    n_cells_board = bit_count(mask);
    update_cipia_vs(mask);

	book = malloc(sizeof(Book));
    book_init(book);
    book->options.level = n_cells_board;
    book->options.n_empties = 61 - 15;
    book->options.midgame_error = 0;
    book->options.endcut_error = 0;
    book->P = board->player;
    book->O = board->opponent;
    book->mask = mask;
    player = BLACK;
    gamepos = 0;

    hash_init(ht, 1u<<20);

    Book *tmp = malloc(sizeof(Book));
    book_init(tmp);
    if( book_load(tmp, "/book/input.dat") ){
        book_free(book);
        free(book);
        book = tmp;

        board->player = book->P;
        board->opponent = book->O;
        gamepos = 0;
        player = BLACK;

        n_cells_board = bit_count(book->mask);
        update_cipia_vs(book->mask);

        FILE* fp = fopen("output.txt", "w");
        char buf[128];
        fprintf(fp, "%s", board_to_string_c(board, player, buf, book->mask));
        fclose(fp);
    } else {
        book_free(tmp);
        free(tmp);
        if( play_flag ) {
//            printf("failed to load book file [%s].\n", cur);
            return -1;
        }
    }

    return 0;
}
