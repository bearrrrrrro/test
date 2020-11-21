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

int pipe_fd[4][2];
FILE *fps[4];
int host_id, key, depth;

void init() {
    int i;
    for (i = 0; i < 4; i++) {
        if (pipe(pipe_fd[i]) < 0) ERR_EXIT("pipe()");
    }
    int j;
    for (j = 0; j < 2; j++) {
        pid_t pid = fork();
        if (pid > 0) { // parent
            fps[j*2 + 1] = fdopen(pipe_fd[j*2 + 1][0], "r");
            fps[j*2 + 0] = fdopen(pipe_fd[j*2 + 0][1], "w");
            close(pipe_fd[j*2 + 0][0]);
            close(pipe_fd[j*2 + 1][1]);
        }
        else if (pid == 0) {
            close(pipe_fd[j*2 + 1][0]);
            close(pipe_fd[j*2 + 0][1]);
            dup2(pipe_fd[j*2 + 0][0], STDIN_FILENO);
            dup2(pipe_fd[j*2 + 1][1], STDOUT_FILENO);
            close(pipe_fd[j*2 + 0][0]);
            close(pipe_fd[j*2 + 1][1]);
            char shost_id[8], shost_key[16], sdepth[8];
            sprintf(sdepth, "%d", depth+1);
            execl("./host", "./host", shost_id, shost_key, sdepth, NULL);
        }
        else {
            ERR_EXIT("fork()");
        }
    }
}

void root_host() {
    int fifo_in_fd, fifo_out_fd;
    char filename[32];
    sprintf(filename, "fifo_%d.tmp", host_id);
    fifo_in_fd = open(filename, O_RDONLY);
    fifo_out_fd = open("fifo_0.tmp", O_WRONLY);
    dup2(fifo_in_fd, STDIN_FILENO);
    dup2(fifo_out_fd, STDOUT_FILENO);
    close(fifo_in_fd);
    close(fifo_out_fd);
    while (1) {
        int root_player_ids[8];
        int root_bids[8];
        int scores[8] = {0};
        int ranks[8] = {1, 1, 1, 1, 1, 1, 1, 1};
        int i, j;
        for (i = 0; i < 8; i++) {
            scanf("%d", &root_player_ids[i]);
        }
        for (j = 0; j < 2; j++) {
            fprintf(fps[j*2], "%d %d %d %d\n",
                root_player_ids[4*j+0],
                root_player_ids[4*j+1],
                root_player_ids[4*j+2],
                root_player_ids[4*j+3]
            );
            fflush(fps[j*2]);
        }
        if (root_player_ids[0] == -1) {
            wait(NULL);
            wait(NULL);
            for (i = 0; i < 4; i++) fclose(fps[i]);
            exit(0);
        }
        
        // 10 rounds
        for (i = 0; i < 10; i++) {
            int in_players[2], in_bids[2];
            fscanf(fps[1], "%d %d", &in_players[0], &in_bids[0]);
            fscanf(fps[3], "%d %d", &in_players[1], &in_bids[1]);

            int winner = (in_bids[0] > in_bids[1] ? 0 : 1);
            int win_player = in_players[winner];
            int win_bid = in_bids[winner];
            int win_index;
            for (win_index = 0; win_index < 8; win_index++) {
                if (root_player_ids[win_index] == win_player) {
                    scores[win_index]++;
                    break;
                }
            }
        }

        // ranking
        for (i = 0; i < 8; i++) {
            for (j = 0; j < 8; j++) {
                if (scores[i] < scores[j]) ranks[i]++;
            }
        }
        // result
        printf("%d\n%d %d\n%d %d\n%d %d\n%d %d\n%d %d\n%d %d\n%d %d\n%d %d\n",
        key, root_player_ids[0], ranks[0],
        root_player_ids[1], ranks[1],
        root_player_ids[2], ranks[2],
        root_player_ids[3], ranks[3],
        root_player_ids[4], ranks[4],
        root_player_ids[5], ranks[5],
        root_player_ids[6], ranks[6],
        root_player_ids[7], ranks[7]
        );
        fflush(stdout);
    }
}

void mid_host() {
    while (1) {
        int players[4];
        scanf("%d %d %d %d", &players[0], &players[1], &players[2], &players[3]);
        fprintf(fps[0], "%d %d\n", players[0], players[1]);
        fflush(fps[0]);
        fprintf(fps[2], "%d %d\n", players[2], players[3]);
        fflush(fps[2]);
        int i;
        if (players[0] == -1) {
            wait(NULL);
            wait(NULL);
            for (i = 0; i < 4; i++) fclose(fps[i]);
            exit(0);
        }
        for (i = 0; i < 10; i++) {
            int in_players[2], in_bids[2];
            fscanf(fps[1], "%d %d", &in_players[0], &in_bids[0]);
            fscanf(fps[3], "%d %d", &in_players[1], &in_bids[1]);
            int winner = (in_bids[0] > in_bids[1] ? 0 : 1);
            int win_player = in_players[winner];
            int win_bid = in_bids[winner];
            printf("%d %d\n", win_player, win_bid);
            fflush(stdout);
        }
    }
}

void leaf_host() {
    while (1) {
        int player_ids[2];
        scanf("%d %d", &player_ids[0], &player_ids[1]);
        if (player_ids[0] == -1) exit(0); // end
        int i;
        for (i = 0; i < 4; i++) {
            if (pipe(pipe_fd[i]) < 0) {
                ERR_EXIT("pipe()");
            }
        }
        int j;
        for (j = 0; j < 2; j++) { // initialize players
            pid_t pid = fork();
            if (pid > 0) {
                fps[j*2+0] = fdopen(pipe_fd[j*2+0][1], "w");
                fps[j*2+1] = fdopen(pipe_fd[j*2+1][0], "r");
                close(pipe_fd[j*2+0][0]);
                close(pipe_fd[j*2+1][1]);
            }
            else if (pid == 0) {
                close(pipe_fd[j*2+1][0]);
                close(pipe_fd[j*2+0][1]);
                dup2(pipe_fd[j*2+0][0], STDIN_FILENO);
                dup2(pipe_fd[j*2+1][1], STDOUT_FILENO);
                close(pipe_fd[j*2+0][0]);
                close(pipe_fd[j*2+1][1]);
                char tmpbuf[8];
                sprintf(tmpbuf, "%d", player_ids[j]);
                execl("./player", "./player", tmpbuf, NULL);
            }
            else {
                ERR_EXIT("fork()");
            }
        }
        for (i = 0; i < 10; i++) {
            int in_players[2], in_bids[2];
            fscanf(fps[1], "%d %d", &in_players[0], &in_bids[0]);
            fscanf(fps[3], "%d %d", &in_players[1], &in_bids[1]);
            int winner = (in_bids[0] > in_bids[1] ? 0 : 1);
            int win_player = in_players[winner];
            int win_bid = in_bids[winner];
            printf("%d %d\n", win_player, win_bid);
            fflush(stdout);
        }
        wait(NULL);
        wait(NULL);
        for (i = 0; i < 4; i++) fclose(fps[i]);
    }
}

int main(int argc, char **argv) {
    if (argc != 4) {
        fprintf(stderr, "usage: %s [host_id] [key] [depth]\n", argv[0]);
        exit(1);
    }
    host_id = atoi(argv[1]);
    key = atoi(argv[2]);
    depth = atoi(argv[3]);
    if (depth < 2) {
        init();
        if (depth == 0) {
            root_host();
        }
        else if (depth == 1) {
            mid_host();
        }
    }
    else if (depth == 2) {
        leaf_host();
    }
    else {
        fprintf(stderr, "depth error: depth must be 0 or 1 or 2\n");
        exit(1);
    }
    return 0;
}