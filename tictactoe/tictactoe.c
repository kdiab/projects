#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

char board[10] = {'x', '1', '2', '3', '4', '5', '6', '7', '8', '9'};
int moves = 0;
void drawBoard(char *a){
	printf("\n	Tic Tac Toe\n\nPlayer 1 (X) - Player 2 (O)\n\n");
		int count = 1;
	for (int t = 0; t < 3; t++){
		printf("	");
		for (int i = 0; i < 11; i++){
			if (i == 3 || i == 7){
				printf("|");
				}
			else{
				printf(" ");				
				}
		}
		printf("\n	");
		for (int j = 0; j < 11; j++){
			if (j == 3 || j == 7) {
				printf("|");
			}
			else if (j == 1 || j == 5 || j == 9){
				printf("%c", a[count]);
				count++;
			}
			else {
				printf(" ");
			}
		}
		printf("\n	");
		for (int k = 0; k < 11; k++){
			if (k == 3 || k == 7){
				printf("|");
				}
			else if (count < 8){
				printf("_");				
				}
			else {
				printf(" ");
			}
		}
		printf("\n");
	}
	printf("\n");
}

char switchPlayer(char a) {
	if (a == 'x'){
		return 'o';
	}
	return 'x';
	}

bool validInput(int p){
		if (p < 1) return false;
		if (p > 9) return false;
		if (p > 0 || p < 10){
			if (board[p] == 'x' || board[p] == 'o'){
				return false;
				}
			return true;
	}
}

void playerInput(char *a){
	int input;
	if (a[0] == 'x'){
		printf("Player 1, enter a number: ");
	}
	else {
		printf("Player 2, enter a number: ");
		}
	scanf("%d", &input);
	while (!validInput(input)){
			printf("Invalid input, please enter a valid number: ");
			scanf("%d", &input);
		}
	a[input] = a[0];
	a[0] = switchPlayer(a[0]);
	moves++;
	}

char checkWinner(char *a){
	if (a[1] == 'x' && a[2] == 'x' && a[3] == 'x') return 'x'; 
	else if (a[4] == 'x' && a[5] == 'x' && a[6] == 'x') return 'x'; 
	else if (a[7] == 'x' && a[8] == 'x' && a[9] == 'x') return 'x'; 
	else if (a[1] == 'x' && a[4] == 'x' && a[7] == 'x') return 'x'; 
	else if (a[2] == 'x' && a[5] == 'x' && a[8] == 'x') return 'x'; 
	else if (a[3] == 'x' && a[6] == 'x' && a[9] == 'x') return 'x'; 
	else if (a[1] == 'x' && a[5] == 'x' && a[9] == 'x') return 'x'; 
	else if (a[7] == 'x' && a[5] == 'x' && a[3] == 'x') return 'x';

	else if (a[1] == 'o' && a[2] == 'o' && a[3] == 'o') return 'o'; 
	else if (a[4] == 'o' && a[5] == 'o' && a[6] == 'o') return 'o'; 
	else if (a[7] == 'o' && a[8] == 'o' && a[9] == 'o') return 'o'; 
	else if (a[1] == 'o' && a[4] == 'o' && a[7] == 'o') return 'o'; 
	else if (a[2] == 'o' && a[5] == 'o' && a[8] == 'o') return 'o'; 
	else if (a[3] == 'o' && a[6] == 'o' && a[9] == 'o') return 'o'; 
	else if (a[1] == 'o' && a[5] == 'o' && a[9] == 'o') return 'o'; 
	else if (a[7] == 'o' && a[5] == 'o' && a[3] == 'o') return 'o'; 
	else return 'u';
}

void redrawScreen(char *a){
	system("clear");
	drawBoard(a);
	playerInput(a);
}

int main(){
		while(moves != 9){
			redrawScreen(board);
			if (moves > 3) {
				if (checkWinner(board) == 'x'){
					system("clear");
					drawBoard(board);
					printf("\aPlayer 1 wins!\n");
					return 0;
				}
				if (checkWinner(board) == 'o'){
					system("clear");
					drawBoard(board);
					printf("\aPlayer 2 wins!\n");
					return 0;
				}
			}
	}
	system("clear");
	drawBoard(board);
	printf("DRAW\n");
	return 0;
}
