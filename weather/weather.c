#include <stdio.h>
float WEATHER[5][12] = {
	{4.3, 4.3, 4.3, 3.0, 2.0, 1.2, 0.2, 0.2, 0.4, 2.4, 3.5, 6.6},
	{8.5, 8.2, 1.2, 1.6, 2.4, 0.0, 5.2, 0.9, 0.3, 0.9, 1.4, 7.3},
	{9.1, 8.5, 6.7, 4.3, 2.1, 0.8, 0.2, 0.2, 1.1, 2.3, 6.1, 8.4},
	{7.2, 9.9, 8.4, 3.3, 1.2, 0.8, 0.4, 0.0, 0.6, 1.7, 4.3, 6.2},
	{7.6, 5.6, 3.8, 2.8, 3.8, 0.2, 0.0, 0.0, 0.0, 1.3, 2.6, 5.2}
};

float monthlyAvg[12];
float yearlyAvg[5];
char *MONTHS[] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"}; 
	
int main() {
	for (int i = 0; i < 5; i++) {
		float yearlySum = 0;
		for (int x = 0; x < 12; x++) {
			yearlySum += WEATHER[i][x];
			monthlyAvg[x] += WEATHER[i][x];
		}
		yearlyAvg[i] = yearlySum;
	}


	printf("YEAR	Rainfall (inches)\n");
	int year = 2010;
	float fuckThis;
	for (int d = 0; d < 5; d++) {
		fuckThis += yearlyAvg[d];
	}
	for (int h = 0; h < 5; h++) {
		printf("%d	%0.1f\n", year, yearlyAvg[h]);
		year++;
	}
	printf("\nThe yearly average is %0.1f inches.\n\n", fuckThis/5);

	printf("MONTHLY AVERAGES:\n\n");
	for (int j = 0; j < 12; j++) {
		printf("%s ", MONTHS[j]);
		monthlyAvg[j] /= 5;
	}
		printf("\n");
	for (int k = 0; k < 12; k++) {
		printf("%0.1f ", monthlyAvg[k]);
	}
}

