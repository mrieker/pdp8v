
//   comand[63:62] = 00: done; 01: restart; 10: read; 11: write [61:54]

//   status[63] = busy
//   status[62] = error
//   status[55:00] = shifted in read bytes

module i2cmaster (CLOCK, RESET, wrcmd, command, status, sclo, sdao, sdai, state, counthi);
    input CLOCK, RESET, wrcmd, sdai;
    output reg sclo, sdao;
    input[63:00] command;
    output reg[63:00] status;

    reg[63:00] comand;
    reg[7:0] count;
    wire[2:0] countlo;
    output[4:0] counthi;
    output reg[2:0] state;

    assign countlo = count[2:0];
    assign counthi = count[7:3];

    localparam[2:0] IDLE  = 0;
    localparam[2:0] START = 1;
    localparam[2:0] BEGIN = 2;
    localparam[2:0] READ  = 3;
    localparam[2:0] WRITE = 4;
    localparam[2:0] STOP  = 5;

    always @(posedge CLOCK or posedge RESET) begin
        if (RESET) begin
            comand <= 0;
            sclo   <= 1;
            sdao   <= 1;
            state  <= IDLE;
            status <= 0;
        end else if (wrcmd) begin
            // command register being written
            comand <= command;
            count  <= 0;
            sclo   <= 1;
            sdao   <= 1;
            state  <= START;
            status[63:62] <= 2'b10;
        end else begin
            case (state)

                // send out start bit, starts with clock and data both high
                // wait 80nS, data down
                // wait 80nS, clock down
            START:
                begin
                    if (countlo != 7) begin
                        count <= count + 1;
                    end else begin
                        case (count[6:3])
                        0:
                            begin
                                count <= count + 1;
                                sdao  <= 0;
                            end
                        1:
                            begin
                                count <= count + 1;
                                sclo  <= 0;
                            end
                        2:
                            begin
                                state <= BEGIN;
                            end
                        endcase
                    end
                end

                // begin processing command in comand[63:...]
                // assumes sclo = 0
            BEGIN:
                begin
                    count  <= 0;
                    case (comand[63:62])
                    2'b00:  // 00 - start sending STOP bit
                        begin
                            sdao  <= 0;
                            state <= STOP;
                        end
                    2'b01:  // 01 - start sending another START bit
                        begin
                            sclo  <= 1;
                            sdao  <= 1;
                            state <= START;
                        end
                    2'b10:  // 10 - start reading 8 bits from slave
                        begin
                            sdao  <= 1;
                            state <= READ;
                        end
                    2'b11:  // 11 - start sending 8 bits to slave
                        begin
                            sdao  <= comand[61];
                            state <= WRITE;
                        end
                    endcase
                    comand <= { comand[61:00], 2'b00 };
                end

                // send stop bit
                // clock, count and data all zero to start
                // wait 80nS, clock up
                // wait 80nS, data up
                // wait 80nS, done
            STOP:
                begin
                    if (countlo != 7) begin
                        count <= count + 1;
                    end else begin
                        case (count[6:3])
                        0:
                            begin
                                count <= count + 1;
                                sclo <= 1;
                            end
                        1:
                            begin
                                count <= count + 1;
                                sdao  <= 1;
                            end
                        2:
                            begin
                                state <= IDLE;
                                status[63:62] <= 2'b00;
                            end
                        endcase
                    end
                end

                // clock and count all zero to start
                // data is one to start
                // repeat 7 times:
                //  wait 80nS, clock up
                //  wait 80nS, clock down, shift in data bits 7..1
                // wait 80nS, clock up
                // wait 80nS, clock down, shift in data bit 0, data down
                // wait 80nS, clock up
                // wait 80nS, clock down
                // wait 80nS, done
            READ:
                begin
                    if (countlo != 7) begin
                        count <= count + 1;
                    end else begin
                        case (counthi)
                        0, 2, 4, 6, 8, 10, 12, 14:
                            begin
                                count <= count + 1;
                                sclo  <= 1;
                            end
                        1, 3, 5, 7, 9, 11, 13:
                            begin
                                count <= count + 1;
                                sclo  <= 0;
                                status[55:00] = { status[54:00], sdai };
                            end
                        15:
                            begin
                                count <= count + 1;
                                sclo  <= 0;
                                sdao  <= 0;
                                status[55:00] = { status[54:00], sdai };
                            end
                        16:
                            begin
                                count <= count + 1;
                                sclo  <= 1;
                            end
                        17:
                            begin
                                count <= count + 1;
                                sclo  <= 0;
                            end
                        18:
                            begin
                                state <= BEGIN;
                            end
                        endcase
                    end
                end

                // clock and count all zero to start
                // data is data bit 7 to start
                // repeat 7 times:
                //  wait 80nS, clock up
                //  wait 80nS, clock down
                //  wait 80nS, send data bits 6..0
                // wait 80nS, clock up
                // wait 80nS, clock down
                // wait 80nS, data up
                // wait 80nS, clock up
                // wait 80nS, if data up
                //              clear busy
                //              set error
                //              go idle
                //            clock down
                // wait 80nS, done
            WRITE:
                begin
                    if (countlo != 7) begin
                        count <= count + 1;
                    end else begin
                        case (counthi)
                        0, 3, 6, 9, 12, 15, 18, 21:
                            begin
                                count <= count + 1;
                                sclo  <= 1;
                            end
                        1, 4, 7, 10, 13, 16, 19, 22:
                            begin
                                count <= count + 1;
                                sclo  <= 0;
                            end
                        2, 5, 8, 11, 14, 17, 20:
                            begin
                                comand <= { comand[62:00], 1'b0 };
                                count  <= count + 1;
                                sdao   <= comand[62];
                            end
                        23:
                            begin
                                comand <= { comand[62:00], 1'b0 };
                                count  <= count + 1;
                                sdao   <= 1;
                            end
                        24:
                            begin
                                count <= count + 1;
                                sclo  <= 1;
                            end
                        25:
                            begin
                                if (sdai) begin
                                    comand[63] <= 1'b0;
                                    status[63:62] <= 2'b01;
                                    state <= IDLE;
                                end else begin
                                    count <= count + 1;
                                    sclo  <= 0;
                                end
                            end
                        26:
                            begin
                                state <= BEGIN;
                            end
                        endcase
                    end
                end

            STOP:
                begin
                    if (countlo != 7) begin
                        count <= count + 1;
                    end else begin
                        case (counthi)
                        0:
                            begin
                                count <= count + 1;
                                sclo  <= 1;
                            end
                        1:
                            begin
                                count <= count + 1;
                                sdao  <= 1;
                            end
                        2:
                            begin
                                state <= IDLE;
                                status[63:62] <= 2'b00;
                            end
                        endcase
                    end
                end
            endcase
        end
    end
endmodule
