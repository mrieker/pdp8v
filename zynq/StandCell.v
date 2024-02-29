
module StandCell (U, _Q, D);
    input   U;
    output _Q;
    input   D;

///*
    reg _Q;
    reg[9:0] counter;

    always @(posedge U) begin
        if (D) begin
            // tPHL = 0.4uS (40 * 10nS)
            if (counter > 39) begin
                counter <= 39;
            end else if (counter != 0) begin
                counter <= counter - 1;
            end else begin
                _Q <= 0;
            end
        end else begin
            // tPLH = 4uS (400 * 10nS)
            if (counter > 399) begin
                _Q <= 1;
            end else begin
                counter <= counter + 1;
            end
        end
    end
//*/

endmodule
