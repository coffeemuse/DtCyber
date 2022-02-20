NCCCTS
*IDENT NCCCTS  KEJ.
*/
*/ THIS MODSET ENABLES *1MT* AND *1LT* TO SUPPORT READING/WRITING
*/ LONG BLOCKS ON CTS DEVICES USING NON-180 IOU'S.
*/
*DECK 1MT 
*I 1MT.9917
          LDD    HP 
          LPC    7700        ALLOW *1LT* TO TEST FOR CTS
          RAM    CALL+2
*D 1MT51.97,1MT51.98
          AJM    *,CH        WAIT FOR *1LT* TO DCN CHANNEL
          PSN
          PSN
 CLB3     IJM    CLB8,CH     IF TRANSFER COMPLETE 
          EJM    CLB3,CH     IF DATA NOT AVAILABLE
*D 274L797.3167,274L797.3169
 CLB4     DCN    CH+40       INDICATE TO *1LT* TO CONTINUE READ
          ZJN    CLB9        IF TRANSFER NOT COMPLETE
*D 1MT51.99
 CLB9     ACN    CH 
*D 274L797.3208
*D 1MT51.102
 RLI3     AJM    *,CH        WAIT FOR *1LT* TO DCN CHANNEL
*D 274L797.3307,1MT51.104
          EJM    RLI4,CH     IF DATA NOT AVAILABLE
*D 274L797.3309,274L797.3311
 RLI5     DCN    CH+40       TELL *1LT* TO START
          ZJN    RLI5.1      IF *1MT* DID NOT FINISH THE TRANSFER
          LJM    RLI9
  
 RLI5.1   ACN    CH 
 RLI5.2   IJM    RLI9,CH     IF *1MT* FINISHED THE CHANNEL TRANSFER
          EJM    RLI5.2,CH   IF *1LT* NOT TAKING DATA YET
*D 1MT51.105,1MT51.106
          AJM    *,CH        WAIT FOR *1LT* TO DCN CHANNEL
*I 274L797.3323
 RLI7     IJM    RLI17,CH    IF *1LT* FINISHED THE TRANSFER 
          EJM    RLI7,CH     IF DATA NOT AVAILABLE
*I 274L797.3324
          DCN    CH+40
*I 274L797.3332
          ACN    CH 
*I 274L797.3333
          DCN    CH+40
*I 274L797.3397
          STM    RLIA
*I 274L797.4009
          LDD    HP 
          LPC    7700        ALLOW *1LT* TO TEST FOR CTS
          RAM    CALL+2
*I 1MT.18062
          LDD    HP 
          SHN    21-7
          MJN    PRS0        IF CTS
          LDD    HP 
*D 1MT.18065
 PRS0     AOD    T3 
*D 21657
 PRS4     LDD    IR+2
          SHN    21-7
          MJN    PRS4.1      IF CTS
          LDC    0
*D 21661
 PRS4.1   LDC    OPA         SET ATS OUTPUT ROUTINE
*D 21672
 PRS7     LDD    IR+2
          SHN    21-7
          MJN    PRS8.1      IF CTS
          LDC    0           CHECK CONTROLLER TYPE
*D 21676
 PRS8.1   LDC    NJNI+.RED8-.REDB  MODIFY INSTRUCTIONS
*/        END OF MODSET.