#include <stdio.h>
#include <stdlib.h>
#include <string.h>
/*nearly all of data will be kept on an array made of this structre*/
struct Hall {
   char  *name;
   char  *movieName;
   unsigned int  width;
   unsigned int  height;
    /*seats has x coordinate(width) at its first dimension, and y(length) at its second.*/
   char **seats;
};

/*handler function decides the command and calls it, gives them necessary parameters.*/
void handler(FILE* in, FILE* out);

/*each command represented as a unique function.*/
void createHall(struct Hall* hall);
void showHall(FILE* out, struct Hall hall);
int sellTicket(FILE* out, struct Hall* hall);
int cancelTicket(FILE* out, struct Hall* hall);
void showStatistics (FILE* out, struct Hall* halls, int hallNumber);

/*helper functions listed below.*/
void seatResolver(char* seatLabel, int* x, int* y);
void removeQuotes(char* str);
int findIndexOfHall(struct Hall* halls, int hallNumber, char* name, char flag);
void drawLine(FILE* out, int width);

/*main is just for file open and close.*/
int main(int argc, char* argv[])
{
    char const* const fileName = argv[1];
    FILE *in, *out;
    in = fopen(fileName, "r");
    out = fopen("output.txt", "w+");

    handler(in,out);

    fclose(in);
    fclose(out);
    return 0;
}

void handler(FILE* in, FILE* out) {
    /*This function has 2 main job:
     *First, it decide command and run the necessary function
     *Second, it declares and maintain data of halls as a dynamic data struction.*/

    char line[300], *token;
    /*maxHallSize represents size of array of hall. it will dynamicly grow.*/
    int hallNumber=0, maxHallSize=2;
    struct Hall *halls =  malloc(maxHallSize * sizeof(struct Hall));

    while (fgets(line, sizeof(line), in)) {
        /*First token is command, one of below ifs will execute.*/
        /*To parse line \r character will be necessary if input.txt produced in windows,
         *because return carriage in linux&unix=\n but in windows=\r\n.*/
        token = strtok(line, " \r\n");

        if(strcmp(token,"CREATEHALL")==0) {
            /*below, hall dynamic array will reallocate and grow as new halls added.*/
            if(++hallNumber > maxHallSize) {
                maxHallSize *= 2;
                halls = (struct Hall *) realloc(halls, maxHallSize * sizeof(struct Hall));
            }
            /*hallNumber-1 indicates to array index of the hall.*/
            createHall(&halls[hallNumber-1]);
        }

        else if(strcmp(token,"BUYTICKET")==0) {
            token = strtok('\0', " \n");
            if (strcmp(token,"\"\"")==0 || strcmp(token,"")==0) {
                fprintf(out, "ERROR: Movie name cannot be empty\n");
                continue;
            }
            removeQuotes(token);
            /*4. argument(flag) indicates that search will be done by movie name, not hall name.*/
            int i = findIndexOfHall(halls,hallNumber,token,'m');
            if(i == -1) {
                fprintf(out, "ERROR: Movie name \"%s\" is incorrect\n", token);
                continue;
            }
            sellTicket(out,&halls[i]);
        }

        else if(strcmp(token,"CANCELTICKET")==0) {
            token = strtok('\0', " \n");
            if (strcmp(token,"\"\"")==0 || strcmp(token,"")==0) {
                fprintf(out, "ERROR: Movie name cannot be empty\n");
                continue;
            }
            removeQuotes(token);
            /*4. argument(flag) indicates that search will be done by movie name, not hall name.*/
            int i = findIndexOfHall(halls,hallNumber,token,'m');
            if(i == -1) {
                fprintf(out, "ERROR: Movie name \"%s\" is incorrect\n", token);
                continue;
            }
            cancelTicket(out,&halls[i]);
        }

        else if(strcmp(token,"SHOWHALL")==0) {
            token = strtok('\0', " \n");
            if (strcmp(token,"\"\"")==0 || strcmp(token,"")==0) {
                fprintf(out, "ERROR: Hall name cannot be empty\n");
                continue;
            }
            removeQuotes(token);
            /*below function will search for given hall by name and returns its index as i.
            4. variable is a flag, 'h' stands for "search by hall name.*/
            int i = findIndexOfHall(halls,hallNumber,token,'h');
            if(i == -1) {
                fprintf(out, "ERROR: Hall name \"%s\" is incorrect\n", token);
                continue;
            }
            showHall(out,halls[i]);
        }

        else if(strcmp(token,"STATISTICS")==0) {
            showStatistics(out,halls,hallNumber);
        }
        else fprintf(out, "Wrong command given.\n");
    }
    free(halls);

}

void showStatistics (FILE* out, struct Hall* halls, int hallNumber) {
    fprintf(out, "Statistics\n");
    int i,j,k,studentCounter=0,fullFareCounter=0,sum;
    for( i=0; i<hallNumber; i++, studentCounter=0, fullFareCounter=0 ) {
        for(j=0; j<halls[i].width; j++) {
            for(k=0; k<halls[i].height; k++){
                if(halls[i].seats[j][k] == 's')
                    studentCounter++;
                if(halls[i].seats[j][k] == 'f')
                    fullFareCounter++;
            }
        }
        sum = 7*studentCounter + 10*fullFareCounter;
        fprintf(out, "%s %d student(s), %d full fare(s), sum:%d TL\n",
            halls[i].movieName, studentCounter, fullFareCounter, sum);
    }
}

int cancelTicket(FILE* out, struct Hall* hall){
    char* seatLabel = strtok('\0', " \r\n");
    /*int i, length=strlen(seatLabel);
    for(i=0; i<length-2; i++) {
        printf("%d-%c (%d)\n", i,seatLabel[i], length);
    }
    seatLabel[i]='\0';*/
    int x,y;
    seatResolver(seatLabel,&x,&y);

    /*we need to handle 2 more error types*/
    if(hall->width <= x || hall->height <= y) {
        fprintf(out, "ERROR: Seat %s is not defined at %s\n", seatLabel, hall->name);
        return -1;
    }
    if (hall->seats[x][y] != 'f' && hall->seats[x][y] != 's' ) {
        fprintf(out, "ERROR: Seat %s in %s was not sold.\n", seatLabel, hall->name);
        return -2;
    }
    /*At that point, we know we checked for 4 kind of errors and now we are sure its safe to approve this operation.*/
    hall->seats[x][y] = '\0';
    fprintf(out, "%s [%s] Purchase cancelled. Seat %s is now free.\n", hall->name, hall->movieName, seatLabel);
    return 0;
}

int sellTicket(FILE* out, struct Hall* hall){
    char *seatLabel = strtok('\0', " \n");
    int x,y;
    seatResolver(seatLabel,&x,&y);

    /*check whether student or fullfare price.*/
    char* ticketTypeString = strtok('\0', " \n");
    char ticketType = (strcmp(ticketTypeString,"Student")==0) ? 's' : 'f';

    int seatRequest = atoi(strtok('\0', " \n"));

    /*we need to handle 2 more error types*/
    /*the reason of -1 below, minimum 1 seatrequest should always be but the point is to count if there is more.*/
    if(hall->width <= x+seatRequest-1 || hall->height <= y) {
        fprintf(out, "ERROR: Seat %s is not defined at %s\n", seatLabel, hall->name);
        return -1;
    }
    int i=0;
    for(i=0; i<seatRequest; i++ ) {
        if (hall->seats[x+i][y] == 'f' || hall->seats[x+i][y] == 's' ) {
            fprintf(out, "ERROR: Specified seat(s) in %s are not available! They have been already taken.\n", hall->name);
            return -2;
        }
    }
    /*At that point, we know we checked for 4 kind of errors and now we are sure its safe to approve this operation.*/
    hall->seats[x][y] = ticketType;
    /*If more seat purchased:*/
    for(i=1; i<seatRequest; i++ ) {
        hall->seats[x+i][y] = ticketType;
        sprintf(seatLabel, "%s%s%c%d", seatLabel, ",", (char)x+i+65, y+1);
    }
    fprintf(out,"%s [%s] Seat(s) %s successfully sold.\n", hall->name, hall->movieName, seatLabel);
    return 0;
}

void createHall(struct Hall* newHall){
    /*temp variable gets hall name and movie name orderly.*/
    char *temp = strtok('\0', " \r\n");
    removeQuotes(temp);
    newHall->name = malloc ( strlen(temp) + 1);
    strcpy(newHall->name, temp);

    temp = strtok('\0', " \r\n");
    removeQuotes(temp);
    newHall->movieName = malloc ( strlen(temp) + 1);
    strcpy(newHall->movieName, temp);

    newHall->width = atoi(strtok('\0', " \r\n"));

    newHall->height = atoi(strtok('\0', " \r\n"));

	/*necessary memory for seats** will be allocated by width and height information.*/
    int i;
    newHall->seats = calloc ( newHall->width, sizeof(char*));
    for(i=0;i<newHall->width;i++)
        newHall->seats[i] = calloc ( newHall->height,sizeof(char));

}

void showHall(FILE* out, struct Hall hall) {
    int i,j,x=hall.width, y=hall.height;

    fprintf(out, "%s sitting plan\n", hall.name);
    /*drawLine, a little helper function for better and elegant reading.*/
    drawLine(out,x);

    for(i=0; i<y; i++){
        /*below if-else: for 1 digits numbers we have a space, for 2 digits we havent.*/
        if(i+1<10){
            fprintf(out, "%d |",i+1);
        }
        else{
            fprintf(out, "%d|",i+1);
        }
        for(j=0; j<x; j++) {
            /*there are only two permitted character: full and student as f and s.
            empty seats presented as space.*/
            if(hall.seats[j][i] == 'f'){
                fprintf(out, "f");
            }
            else if(hall.seats[j][i] == 's'){
                fprintf(out, "s");
            }
            else {
                fprintf(out, " ");
            }
            fprintf(out, "|");
        }
        fprintf(out, "\n");
        drawLine(out,x);
    }

    fprintf(out, "  ");
    for(j=0; j<x; j++){
        /*In ASCII table, 'A' character has decimal no 65, and orderly goes to 90 which is 'Z'.*/
        fprintf(out, " %c", 65+j);
    }
    fprintf(out, "\n");
    for(j=0; j<x-6; j++) {
        fprintf(out, " ");
    }
    fprintf(out, "C U R T A I N\n");
}

void seatResolver(char* seatLabel, int* x, int* y) {
    /*x and y coordinate index of the given seat as "halls[i].seats[x][y]"*/
    *x = seatLabel[0] - 64 - 1;
    if(strlen(seatLabel) == 2) {
        *y = (seatLabel[1] - '0') - 1;
    }
    else {
        *y = (seatLabel[1] - '0')*10 + (seatLabel[2] - '0') - 1;
    }
}

void drawLine(FILE* out, int width) {
    int i;
    fprintf(out, "  ");
    for(i=0; i<width; i++)
        fprintf(out, "--");
    fprintf(out, "-\n");
}

int findIndexOfHall(struct Hall* halls, int hallNumber, char* src, char flag) {
    int i;
    for(i=0; i<hallNumber;i++) {
        /*if flag equals 'h', than we will search by hall name, if not than we will search by movie name.*/
        if(strcmp( flag == 'h' ? halls[i].name : halls[i].movieName,src)==0)
            return i;
    }
    return -1;
}

void removeQuotes(char* str) {
    int i;
    /*in case of infinite loop, iteration was limited by max 100.*/
    for ( i = 0; i<100 ; i++ ) {
        if(str[i+1] == '\"')
            break;
        str[i] = str[i+1];
    }
    str[i] = '\0';
}

