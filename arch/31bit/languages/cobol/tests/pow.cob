       IDENTIFICATION DIVISION.
       PROGRAM-ID. POW.
       DATA DIVISION.
       WORKING-STORAGE SECTION.
       01  WS-BIG      PIC 9(30) VALUE 0.
       01  WS-B18      PIC 9(18) VALUE 0.
       01  WS-X        PIC 9(30) VALUE 0.
       PROCEDURE DIVISION.
           COMPUTE WS-B18 = 2 ** 60.
           DISPLAY "2**60 = " WS-B18.
           COMPUTE WS-BIG = 2 ** 90
               ON SIZE ERROR DISPLAY "SIZE ERROR 2**90"
           END-COMPUTE.
           DISPLAY "2**90 = " WS-BIG.
           MOVE 1 TO WS-X.
           PERFORM 90 TIMES
               COMPUTE WS-X = WS-X * 2
           END-PERFORM.
           DISPLAY "LOOP  = " WS-X.
           COMPUTE WS-BIG = 1237940039285380274899124224 / 1024.
           DISPLAY "DIV   = " WS-BIG.
           STOP RUN.
