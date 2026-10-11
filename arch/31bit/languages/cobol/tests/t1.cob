       IDENTIFICATION DIVISION.
       PROGRAM-ID. T1.
       ENVIRONMENT DIVISION.
       INPUT-OUTPUT SECTION.
       FILE-CONTROL.
           SELECT SRTF ASSIGN TO "srt.tmp".
           SELECT INF  ASSIGN TO "in.dat" ORGANIZATION LINE SEQUENTIAL.
           SELECT OUTF ASSIGN TO "out.dat" ORGANIZATION LINE SEQUENTIAL.
           SELECT RELF ASSIGN TO "rel.dat" ORGANIZATION RELATIVE
               ACCESS DYNAMIC RELATIVE KEY RK.
       DATA DIVISION.
       FILE SECTION.
       SD  SRTF.
       01  S-REC.
           05 S-KEY    PIC X(6).
           05 S-VAL    PIC 9(5).
       FD  INF.
       01  I-REC       PIC X(11).
       FD  OUTF.
       01  O-REC       PIC X(11).
       FD  RELF.
       01  R-REC       PIC X(20).
       WORKING-STORAGE SECTION.
       01  RK          PIC 9(4).
       01  P1          PIC S9(7)V99 COMP-3 VALUE -12345.67.
       01  P2          PIC S9(5)V9(4) COMP-3 VALUE 3.1416.
       01  B1          PIC S9(9) COMP-5 VALUE -987654321.
       01  B2          PIC S9(18) BINARY VALUE 123456789012345678.
       01  R1          PIC S9(9)V99.
       01  E1          PIC $$$,$$$,$$9.99-.
       01  E2          PIC ZZZ,ZZ9.999.
       01  Q           PIC S9(9).
       01  RM          PIC S9(9).
       01  STR         PIC X(40) VALUE "THE QUICK BROWN FOX JUMPS".
       01  W1          PIC X(10).
       01  W2          PIC X(10).
       01  W3          PIC X(10).
       01  CNT         PIC 99 VALUE 0.
       01  OUTS        PIC X(40).
       01  TBL.
           05 TE OCCURS 10 ASCENDING KEY TK INDEXED BY TX.
              10 TK    PIC 99.
              10 TV    PIC X(5).
       01  I           PIC 99.
       01  EOF         PIC X VALUE "N".
       PROCEDURE DIVISION.
           COMPUTE R1 ROUNDED = P1 * P2.
           MOVE R1 TO E1. DISPLAY "P1*P2 " E1.
           COMPUTE R1 = P1 / 7. DISPLAY "P1/7 " R1.
           DIVIDE 1000 BY 7 GIVING Q REMAINDER RM.
           DISPLAY "Q " Q " R " RM.
           COMPUTE B1 = B1 + 1. DISPLAY "B1 " B1.
           COMPUTE B2 = B2 * 3 / 2. DISPLAY "B2 " B2.
           MOVE 1234.5678 TO E2. DISPLAY "E2 " E2.
           INSPECT STR TALLYING CNT FOR ALL "O".
           DISPLAY "O COUNT " CNT.
           INSPECT STR REPLACING ALL "O" BY "0".
           DISPLAY STR.
           UNSTRING STR DELIMITED BY SPACE INTO W1 W2 W3.
           DISPLAY W3 "|" W2 "|" W1.
           STRING W3 DELIMITED BY SPACE "-" W1 DELIMITED BY SPACE
               INTO OUTS.
           DISPLAY OUTS.
           PERFORM VARYING I FROM 1 BY 1 UNTIL I > 10
               COMPUTE TK(I) = I * 3
               MOVE FUNCTION REVERSE("ABCDE") TO TV(I)
           END-PERFORM.
           SEARCH ALL TE AT END DISPLAY "NOT FOUND"
               WHEN TK(TX) = 21 DISPLAY "FOUND 21 AT " TX.
           DISPLAY FUNCTION MAX(3 9 2) " " FUNCTION MIN(3 9 2) " "
               FUNCTION MOD(-7 3) " " FUNCTION REM(-7 3).
           DISPLAY FUNCTION INTEGER-OF-DATE(20261011) " "
               FUNCTION DATE-OF-INTEGER(150000).
           DISPLAY FUNCTION NUMVAL("  -123.45 ") " "
               FUNCTION LENGTH(STR) " " FUNCTION ORD("A").
           DISPLAY FUNCTION PI.
           DISPLAY FUNCTION EXP(1).
           DISPLAY FUNCTION LOG(10).
           DISPLAY FUNCTION SIN(1).
           DISPLAY FUNCTION RANDOM(42).
           OPEN OUTPUT INF.
           MOVE "PEAR  00030" TO I-REC. WRITE I-REC.
           MOVE "APPLE 00010" TO I-REC. WRITE I-REC.
           MOVE "ZEBRA 00050" TO I-REC. WRITE I-REC.
           MOVE "MANGO 00020" TO I-REC. WRITE I-REC.
           CLOSE INF.
           SORT SRTF ON ASCENDING KEY S-KEY USING INF GIVING OUTF.
           OPEN INPUT OUTF.
           PERFORM UNTIL EOF = "Y"
               READ OUTF AT END MOVE "Y" TO EOF
                   NOT AT END DISPLAY "SORTED " O-REC END-READ
           END-PERFORM.
           CLOSE OUTF.
           OPEN OUTPUT RELF.
           PERFORM VARYING RK FROM 1 BY 1 UNTIL RK > 5
               MOVE SPACES TO R-REC
               STRING "REL " RK DELIMITED BY SIZE INTO R-REC
               WRITE R-REC
           END-PERFORM.
           CLOSE RELF.
           OPEN I-O RELF.
           MOVE 3 TO RK. READ RELF. DISPLAY "RREAD " R-REC.
           MOVE "CHANGED" TO R-REC. REWRITE R-REC.
           MOVE 3 TO RK. READ RELF. DISPLAY "RREAD " R-REC.
           CLOSE RELF.
           CALL "SUBP" USING STR.
           DISPLAY "AFTER CALL " STR(1:10).
           STOP RUN.
