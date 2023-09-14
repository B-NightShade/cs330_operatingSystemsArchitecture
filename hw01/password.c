#include <stdio.h>
#include <string.h>

int main(){
	char password[200];
	char correctPassword [] = "hello";
	printf("enter password: ");
	scanf("%s", password);

	if (strcmp(correctPassword, password)==0){
		printf("password Correct!\n");
	}
	else{
		printf("password is wrong!\n");
	}
}
