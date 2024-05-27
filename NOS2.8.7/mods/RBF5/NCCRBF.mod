*IDENT    NCCRBF
*/
*/  MOD TO UPDATE RBF PERFORMANCE TUNING PARAMETERS AND CORRECT
*/  SRU REPORTING TO AVOID OVERFLOWING A 7-DIGIT OUTPUT FIELD.
*/
*/      K. E. JORDAN   5/27/2024
*/
*DELETE IP$COM.6
   DEF TOTDEV#64#;           #MAX NO OF BATCH DEVICES IN USE AT ONE TIM#
*DELETE IP$COM.7
   DEF RESUMETIME#5#;        #TIME TO AUTO RESUME 200 UTS              #
*DELETE PRU0008.3
   DEF SEARCHTIME#5#;        #TIME BETWEEN OUTPUT QUEUE SEARCH         #
*COMPILE  IP$COM
*/
*/ THE FOLLOWING CHANGE MAKES THE RBF *DISPLAY,EX* COMMAND SHOW SRU-S
*/ TO THE NEAREST WHOLE UNIT, AND THIS AVOIDS OVERFLOWING THE 7-DIGIT
*/ OUTPUT FIELD.
*/
*DELETE RB2A999.43
            XCONVERT = XCOD((R$SRU+5000000)/10000000);  # INSERT SRU-S #
*COMPILE DIS
*/ THIS MODSET CONTAINS 23 LINES INCLUDING THIS COMMENT.
