#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <sys/wait.h>
#include <sys/types.h>

#define ERR_EXIT(m) \
     do { \
        perror(m); \
        exit(EXIT_FAILURE); \
    }  while( 0)

#define MAX_PLAYER 15
char buf[BUFSIZ];

// void readline(int fd, char *str) {
//     int k = 0;
//     ssize_t bytes_read;
//     char c;
//     do {
//         c = 0;
//         if ((bytes_read = read(fd, &c, 1)) < 0) {
//             ERR_EXIT("read()");
//         }
//         if (c == '\n' || bytes_read == 0) c = 0;
//         str[k++] = c;
//     } while (c);
// }

void initPlayers(int player_id[2], int fd[4][2], FILE *fps[2][2]) {
    int i;
    for (i = 0; i < 2; i++) {
        pid_t pid = fork();
        if (pid < 0) {
            ERR_EXIT("fork()");
        }
        else if (pid == 0) {
            dup2(fd[i][1], STDOUT_FILENO); // write to parent
            dup2(fd[i | 2][0], STDIN_FILENO); // read from parent
            close(fd[i][0]);
            close(fd[i | 2][1]);
            char *pathName = "./player";
            char tmpstr[32];
            sprintf(tmpstr, "%d", player_id[i]);
            char *args[] = {pathName, tmpstr, NULL};
            execvp(pathName, args);
        }
        else {
            close(fd[i][1]);
            close(fd[i | 2][0]);
            fps[i][0] = fdopen(fd[i][0], "r"); // read from children
            fps[i][1] = fdopen(fd[i | 2][1], "w"); // write to children
        }
    }
}

int score[MAX_PLAYER];
void rankPlayers(int players[8], int ret_rank[8]) {
    int rank[MAX_PLAYER];
    int i, k, cur_rank, cnt = 1;
    for (k = 8; k >= 0; k--) {
        cur_rank = cnt;
        for (i = 0; i < MAX_PLAYER; i++) {
            if (score[i] == k) {
                rank[i] = cur_rank;
                cnt++;
            }
        }
    }
    k = 0;
    for (i = 0; i < MAX_PLAYER; i++) {
        if (score[i] != -1) {
            // fprintf(stderr, "test: %d %d\n", i, rank[i]);
            // printf("%d %d\n", i, rank[i]); // player{x}_id, player{x}_rank
            players[k] = i;
            ret_rank[k] = rank[i];
            k++;
        }
    }
}

int host_id, key, depth;
int main(int argc, char **argv) {
    if (argc != 4) {
        fprintf(stderr, "usage: %s [host_id] [key] [depth]\n", argv[0]);
        exit(1);
    }
    const int num_of_round = 10;
    const int tree_height = 2;

    host_id = atoi(argv[1]);
    key = atoi(argv[2]);
    depth = atoi(argv[3]);
    // fprintf(stderr, "START depth = %d\n", depth);
    int i;

    // fork & pipe & fifo

    int fd[4][2];
    for (i = 0; i < 4; i++) {
        pipe(fd[i]);
    }
    
    FILE *fps[2][2];
    if (depth < tree_height) {
        for (i = 0; i < 2; i++) {
            pid_t pid;
            pid = fork();
            if (pid < 0) {
                ERR_EXIT("fork()");
            }
            else if (pid == 0) { // child
                dup2(fd[i][1], STDOUT_FILENO); // write to parent
                dup2(fd[i | 2][0], STDIN_FILENO); // read from parent
                close(fd[i][0]);
                close(fd[i][1]);
                close(fd[i | 2][0]);
                close(fd[i | 2][1]);
                argv[3][0]++;
                char *pathName = "./host";
                char *args[] = {pathName, argv[1], argv[2], argv[3], NULL};
                execvp(pathName, args);
            }
            else { // parent
                close(fd[i][1]);
                close(fd[i | 2][0]);
                fps[i][0] = fdopen(fd[i][0], "r"); // read from children
                fps[i][1] = fdopen(fd[i | 2][1], "w"); // write to children
            }
        }
    }

    if (depth == 0) {
        sprintf(buf, "fifo_%d.tmp", host_id);
        int infd, outfd;
        infd = open(buf, O_RDONLY);
        outfd = open("fifo_0.tmp", O_RDWR);
        // fprintf(stderr, "8depth = %d\n", depth);
        if (infd < 0 || outfd < 0) {
            ERR_EXIT("open error");
        }
        dup2(infd, STDIN_FILENO); // read from fifo_{host_id}.tmp
        dup2(outfd, STDOUT_FILENO); // write to fifo_0.tmp
        close(infd);
        close(outfd);
    }

    // bid

    while (1) {
        memset(score, -1, sizeof(score));
        // fprintf(stderr, "depth = %d\n", depth);
        if (depth == tree_height) { // leaf host
            int player_id[2];
            scanf("%d %d", &player_id[0], &player_id[1]);
            if (player_id[0] == -1) exit(0);
            initPlayers(player_id, fd, fps);
        }
        else {
            int num_of_players = 1 << (tree_height - depth);
            int player_id;
            int j;
            for (j = 0; j < 2; j++) {
                for (i = 0; i < num_of_players; i++) {
                    scanf("%d", &player_id);
                    // fprintf(stderr, "player_id = %d\n", player_id);
                    if (depth == 0 && player_id != -1) score[player_id] = 0; // set players for root host 
                    fprintf(fps[j][1], "%d%c", player_id, " \n"[i == num_of_players - 1]);
                    fflush(fps[j][1]);
                    // write(fd[j | 2][1], buf, strlen(buf));
                }
            }
            // fprintf(stderr, "player_id = %d\n", player_id);
            if (player_id == -1) {
                for (j = 0; j < 2; j++) {
                    pid_t pid = wait(NULL);
                    if (pid < 0) {
                        sprintf(buf, "depth = %d; wait()", depth);
                        ERR_EXIT(buf);
                    }
                }
                exit(0);
            } 
        }

        // compare bid & ranking

        int round;
        int players[2], bids[2], winner;
        
        for (round = 0; round < num_of_round; round++) {
            for (int i = 0; i < 2; i++) {
                fscanf(fps[i][0], "%d %d", &players[i], &bids[i]);
                // fprintf(stderr, "d %d p %d b %d\n", depth, players[i], bids[i]);
            }
            
            winner = (bids[0] > bids[1] ? 0 : 1);
            if (depth == 0) { // root host
                // fprintf(stderr, "player %d wins with bid %d\n", players[winner], bids[winner]);
                // fprintf(stderr, "player %d loses with bid %d\n", players[~winner&1], bids[~winner&1]);
                score[players[winner]]++;
            }
            else {
                printf("%d %d\n", players[winner], bids[winner]);
                fflush(stdout);
            }
        }

        if (depth == 0) {
            int players[8], ranks[8];
            rankPlayers(players, ranks);
            char buf[1024];
            // fflush(stdout);
            sprintf(buf, "%d\n%d %d\n%d %d\n%d %d\n%d %d\n%d %d\n%d %d\n%d %d\n%d %d\n",
            key, players[0], ranks[0], players[1], ranks[1], players[2], ranks[2], players[3], ranks[3],
             players[4], ranks[4], players[5], ranks[5], players[6], ranks[6], players[7], ranks[7]);
            // fprintf(stderr, "%s", buf);
            fprintf(stdout, "%s", buf);
            fflush(stdout);
        }

        if (depth == tree_height) {
            for (i = 0; i < 2; i++) {
                pid_t pid = wait(NULL);
                if (pid < 0) {
                    sprintf(buf, "leaf host; wait()");
                    ERR_EXIT(buf);
                }
            }
        }
        if (fflush(stdout) < 0) {
            ERR_EXIT("fflush()");
        }
    }
    return 0;
}
