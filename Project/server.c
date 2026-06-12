//Sam Bryan
//I pledge my honor that I have abided by the Stevens Honor System.
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <getopt.h>
#include <sys/select.h>

struct Entry {
    char prompt[1024];
    char options[3][50];
    int answer_idx;
};

struct Player { 
    int fd;
    int score;
    char name[128];
};

int read_questions(struct Entry* arr, char* filename) {
    FILE* fp = fopen(filename, "r");
    if (!fp) return 0;
    int count = 0;
    char line[1024];
    while (fgets(line, sizeof(line), fp) && count < 50) {
        strcpy(arr[count].prompt, line);
        arr[count].prompt[strlen(arr[count].prompt)-1] = '\0';
        fgets(line, sizeof(line), fp);
        char* token = strtok(line, " \n");
        for (int i = 0; i < 3; i++) { 
            strcpy(arr[count].options[i], token);
            token = strtok(NULL, " \n");
        }
        fgets(line, sizeof(line), fp);
        line[strlen(line)-1] = '\0';
        for (int i = 0; i < 3; i++) {
            if (strcmp(arr[count].options[i], line) == 0) {
                arr[count].answer_idx = i;
                break;
            }
        }
        fgets(line, sizeof(line), fp);
        count++;
    }
    fclose(fp);
    return count;
}

int main(int argc, char** argv) {
    char* question_file = "questions.txt";
    char* ip = "127.0.0.1";
    int port = 25555;
    int opt;
    while ((opt = getopt(argc, argv, "f:i:p:h")) != -1) {
        if (opt == 'f')
        {
            question_file = optarg;
        }
        else if (opt == 'i')
        {
            ip = optarg;
        }
        else if (opt == 'p')
        {
            port = atoi(optarg);
        }
        else if (opt == 'h')
        {
            printf("Usage: %s [-f question_file] [-i IP_address] [-p port_number] [-h]\n", argv[0]);
            printf("-f question_file\n");
            printf("-i IP_address\n");
            printf("-p port_number\n");
            printf("-h Display this help info.\n");
            exit(EXIT_SUCCESS);
        }
        else
        {
            fprintf(stderr, "Unknown option: %c received.\n", opt);
            exit(EXIT_FAILURE);
        }
    }
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
        perror("socket");
        return 1;
    }
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = inet_addr(ip);
    addr.sin_port = htons(port);
    if (bind(server_fd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        perror("bind");
        return 1;
    }
    if (listen(server_fd, 3) < 0) {
        perror("listen");
        return 1;
    }
    printf("Welcome to 392 Trivia!\n");
    struct Entry questions[50];
    memset(questions, 0, sizeof(questions));
    int num_questions = read_questions(questions, question_file);
    if (num_questions == 0) {
        fprintf(stderr, "No questions read\n");
        return 1;
    }
    struct Player players[3];
    int num_players = 0;
    fd_set master_set, read_set;
    FD_ZERO(&master_set);
    FD_SET(server_fd, &master_set);
    int max_fd = server_fd;
    while (1) {
        read_set = master_set;
        if (select(max_fd + 1, &read_set, NULL, NULL, NULL) < 0) {
            perror("select");
            return 1;
        }
        if (FD_ISSET(server_fd, &read_set)) {
            struct sockaddr_in client_addr;
            socklen_t addr_len = sizeof(client_addr);
            int client_fd = accept(server_fd, (struct sockaddr*)&client_addr, &addr_len);
            if (client_fd < 0) {
                perror("accept");
                continue;
            }
            if (num_players >= 2) {
                printf("Max connection reached!\n");
                close(client_fd);
            } else {
                printf("New connection detected!\n");
                send(client_fd, "Please type your name:", 22, 0);
                players[num_players].fd = client_fd;
                players[num_players].score = 0;
                FD_SET(client_fd, &master_set);
                if (client_fd > max_fd) max_fd = client_fd;
                num_players++;
            }
        }
        if (num_players == 2) break;
    }
    for (int i = 0; i < num_players; i++) {
        char name[128];
        int len = recv(players[i].fd, name, sizeof(name), 0);
        if (len <= 0) {
            printf("Lost connection!\n");
            return 1;
        }
        name[len-1] = '\0';
        strcpy(players[i].name, name);
        printf("Hi %s!\n", name);
    }
    printf("The game starts now!\n");
    for (int q = 0; q < num_questions; q++) {
        char buffer[2048];
        sprintf(buffer, "Question %d: %s\nPress 1: %s\nPress 2: %s\nPress 3: %s\n",
                q+1, questions[q].prompt,
                questions[q].options[0], questions[q].options[1], questions[q].options[2]);
        for (int i = 0; i < num_players; i++) {
            send(players[i].fd, buffer, strlen(buffer), 0);
        }
        printf("Question %d: %s\n1: %s\n2: %s\n3: %s\n",
               q+1, questions[q].prompt,
               questions[q].options[0], questions[q].options[1], questions[q].options[2]);
        int answered = 0;
        while (!answered) {
            read_set = master_set;
            FD_CLR(server_fd, &read_set);
            if (select(max_fd + 1, &read_set, NULL, NULL, NULL) < 0) {
                perror("select");
                return 1;
            }
            for (int i = 0; i < num_players; i++) {
                if (FD_ISSET(players[i].fd, &read_set)) {
                    char ans[2];
                    int len = recv(players[i].fd, ans, sizeof(ans), 0);
                    if (len <= 0) {
                        printf("Lost connection!\n");
                        return 1;
                    }
                    int choice = ans[0] - '1';
                    if (choice == questions[q].answer_idx) {
                        players[i].score++;
                    } else {
                        players[i].score--;
                    }
                    answered = 1;
                    sprintf(buffer, "Correct answer: %s\n", questions[q].options[questions[q].answer_idx]);
                    for (int j = 0; j < num_players; j++) {
                        send(players[j].fd, buffer, strlen(buffer), 0);
                    }
                    printf("Correct answer: %s\n", questions[q].options[questions[q].answer_idx]);
                    break;
                }
            }
        }
    }
    int max_score = -100;
    for (int i = 0; i < num_players; i++) {
        if (players[i].score > max_score) max_score = players[i].score;
    }
    printf("Congrats, ");
    int first = 1;
    for (int i = 0; i < num_players; i++) {
        if (players[i].score == max_score) {
            if (!first) printf(", ");
            printf("%s", players[i].name);
            first = 0;
        }
    }
    printf("!\n");
    for (int i = 0; i < num_players; i++) {
        close(players[i].fd);
    }
    close(server_fd);
    return 0;
}