       IDENTIFICATION DIVISION.
       PROGRAM-ID. FILES.
       ENVIRONMENT DIVISION.
       INPUT-OUTPUT SECTION.
       FILE-CONTROL.
           SELECT OUTF ASSIGN TO "test.dat"
               ORGANIZATION IS LINE SEQUENTIAL.
       DATA DIVISION.
       FILE SECTION.
       FD  OUTF.
       01  OUT-REC     PIC X(40).
       WORKING-STORAGE SECTION.
       01  WS-N        PIC 99.
       01  WS-EOF      PIC X VALUE "N".
       01  WS-CNT      PIC 999 VALUE 0.
       PROCEDURE DIVISION.
           OPEN OUTPUT OUTF.
           PERFORM VARYING WS-N FROM 1 BY 1 UNTIL WS-N > 10
               STRING "RECORD " WS-N DELIMITED BY SIZE INTO OUT-REC
               WRITE OUT-REC
           END-PERFORM.
           CLOSE OUTF.
           OPEN INPUT OUTF.
           PERFORM UNTIL WS-EOF = "Y"
               READ OUTF AT END MOVE "Y" TO WS-EOF
                   NOT AT END ADD 1 TO WS-CNT
               END-READ
           END-PERFORM.
           CLOSE OUTF.
           DISPLAY "READ BACK " WS-CNT " RECORDS".
           STOP RUN.
