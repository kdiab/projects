#include <stdio.h>

int main() {
	double USER_INPUT = 0.0;
	double RATE = 12.00;
	double OVERTIME = 18.00;

	printf("Enter number of hours worked this week: ");
	scanf("%lf", &USER_INPUT);
	
	double grossPay = 0.0;
	if (USER_INPUT <= 40) {
		grossPay = RATE * USER_INPUT;
	}
	else {
		grossPay = ((USER_INPUT - 40) * OVERTIME) + (40 * RATE);
	}

	double fifteenPercent = 0.0;
	double twentyPercent = 0.0;
	double twentyFivePercent = 0.0;
	double taxedIncome = 0.0;
	if (grossPay <= 300.0) {
		fifteenPercent  = grossPay * 0.15;
		taxedIncome = fifteenPercent;
	}
	else if (grossPay > 300.0 && grossPay <= 450.0) {
		fifteenPercent = 300 * 0.15;
		twentyPercent = (grossPay - 300.0) * 0.20;
		taxedIncome = fifteenPercent + twentyPercent;
	}
	else {
		fifteenPercent = 300 * 0.15;
		twentyPercent = 150 * 0.20;
		twentyFivePercent = (grossPay - 450.0) * 0.25; 
		taxedIncome = fifteenPercent + twentyPercent + twentyFivePercent;
	}

	printf("Gross Pay: %.2lf\nTaxed Income: %.2lf\nNet Pay: %.2lf\n", grossPay, taxedIncome, grossPay - taxedIncome);
}
