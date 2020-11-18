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

void readline(int fd, char *str) {
    int k = 0;
    ssize_t bytes_read;
    char c;
    do {
        c = 0;
        if ((bytes_read = read(fd, &c, 1)) < 0) {
            ERR_EXIT("read()");
        }
        if (c == '\n' || bytes_read == 0) c = 0;
        str[k++] = c;
    } while (c);
}

void initPlayers(char player_id[2][32], int fd[4][2]);

int score[MAX_PLAYER];

void rankPlayers() {
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
    for (i = 0; i < MAX_PLAYER; i++) {
        if (score[i] != -1) {
            fprintf(stderr, "test: %d %d\n", i, rank[i]);
            printf("%d %d\n", i, rank[i]); // player{x}_id, player{x}_rank
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
    int i;

    // fprintf(stderr, "ppid = %d, pid = %d, depth = %d\n", getppid(), getpid(), depth);

    // fork & pipe & fifo

    int fd[4][2];
    for (i = 0; i < 4; i++) {
        pipe(fd[i]);
    }
    
    if (depth < tree_height) {
        for (i = 0; i < 2; i++) {
            // fprintf(stderr, "depth = %d\n", depth);
            pid_t pid;
            pid = fork();
            // fprintf(stderr, "pid = %d\n", pid);
            if (pid < 0) {
                ERR_EXIT("fork()");
            }
            else if (pid == 0) { // child
                // fprintf(stderr, "fork at depth = %d\n", depth);
                dup2(fd[i][1], STDOUT_FILENO); // write to parent
                dup2(fd[i | 2][0], STDIN_FILENO); // read from parent
                close(fd[i][0]);
                close(fd[i | 2][1]);
                argv[3][0]++;
                char *pathName = "./host";
                char *args[] = {pathName, argv[1], argv[2], argv[3], NULL};
                execvp(pathName, args);
            }
            else { // parent
                close(fd[i][1]);
                close(fd[i | 2][0]);
            }
        }
    }

    if (depth == 0) {
        sprintf(buf, "fifo_%d.tmp", host_id);
        int infd, outfd;
        infd = open(buf, O_RDONLY);
        outfd = open("fifo_0.tmp", O_WRONLY);
        if (infd < 0 || outfd < 0) {
            ERR_EXIT("open error");
        }
        dup2(infd, STDIN_FILENO); // read from fifo_{host_id}.tmp
        dup2(outfd, STDOUT_FILENO); // write to fifo_0.tmp
    }

    // bid

    while (1) {
        memset(score, -1, sizeof(score));

        if (depth == tree_height) {
            char player_id[2][32];
            scanf("%s %s", player_id[0], player_id[1]);
            if (strcmp(player_id[0], "-1") == 0) exit(0);
            initPlayers(player_id, fd);
        }
        else {
            int num_of_players = 1 << (tree_height - depth);
            char player_id[32];
            int j;
            for (j = 0; j < 2; j++) {
                for (i = 0; i < num_of_players; i++) {
                    scanf("%s", player_id);
                    int tmp = atoi(player_id);
                    if (depth == 0 && tmp != -1) score[tmp] = 0; // set players for root host 
                    sprintf(buf, "%s%c", player_id, " \n"[i == num_of_players - 1]);
                    write(fd[j | 2][1], buf, strlen(buf));
                }
            }
            if (strcmp(player_id, "-1") == 0) {
                for (j = 0; j < 2; j++) {
                    pid_t pid = wait(NULL);
                    if (pid < 0) {
                        sprintf(buf, "depth = %d; wait()", depth);
                        ERR_EXIT(buf);
                    }
                    // else {
                    //     fprintf(stderr, "wait at depth = %d; child pid = %d\n", depth, pid);
                    // }
                }
                exit(0);
            } 
        }

        // compare bid & ranking

        int round;
        char readbuf[32];
        int players[2], bids[2], winner;

        for (round = 0; round < num_of_round; round++) {
            for (int i = 0; i < 2; i++) {
                readline(fd[i][0], readbuf);
                sscanf(readbuf, "%d %d", &players[i], &bids[i]);
                // if (depth == tree_height)
                //     fprintf(stderr, "%d %d\n", players[i], bids[i]);
            }
            
            winner = (bids[0] > bids[1] ? 0 : 1);
            if (depth == 0) {
                score[players[winner]]++;
            }
            else {
                printf("%d %d\n", players[winner], bids[winner]);
                if (fflush(stdout) < 0) {
                    ERR_EXIT("fflush()");
                }
            }
        }

        if (depth == 0) {
            // fprintf(stderr, "%d\n", key);
            printf("%d\n", key);
            for (i = 0; i < MAX_PLAYER; i++) {
                if (score[i] != -1) {
                    fprintf(stderr, "%d %d\n", i, score[i]);
                }
            }
            rankPlayers();
        }

        if (depth == tree_height) {
            for (i = 0; i < 2; i++) {
                pid_t pid = wait(NULL);
                if (pid < 0) {
                    sprintf(buf, "leaf host; wait()");
                    ERR_EXIT(buf);
                }
                // else {
                //     fprintf(stderr, "pid = %d\n", pid);
                // }
            }
        }
    }

    return 0;
}

void initPlayers(char player_id[2][32], int fd[4][2]) {
    int i;
    // fprintf(stderr, "Init player: %s %s ...\n", player_id[0], player_id[1]);
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
            char *args[] = {pathName, player_id[i], NULL};
            execvp(pathName, args);
        }
        else {
            close(fd[i][1]);
            close(fd[i | 2][0]);
        }
    }
}