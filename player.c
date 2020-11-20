#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>

#define ERR_EXIT(m) \
     do { \
        perror(m); \
        exit(EXIT_FAILURE); \
    }  while( 0)

int main(int argc, char **argv) {
    
    if (argc != 2) {
        fprintf(stderr, "usage: %s [player_id]\n", argv[0]);
        exit(1);
    }
    int player_id = atoi(argv[1]);
    
    // The bid must follow the rules for TAs to check your work:

    const int bid_list[21] = {
        20, 18, 5, 21, 8, 7, 2, 19, 14, 13,
        9, 1, 6, 10, 16, 11, 4, 12, 15, 17, 3
    };

    int round, bid;

    for (round = 1; round <= 10; round++) {
        bid = bid_list[player_id + round - 2] * 100;
        printf("%d %d\n", player_id, bid);
    }
    if (fflush(stdout) < 0) {
        ERR_EXIT("fflush()");
    }

    return 0;
}