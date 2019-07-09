
module QD1 (
	clk_50_clk,
	reset_reset_n,
	audio_mclk_clk,
	sdram_0_addr,
	sdram_0_ba,
	sdram_0_cas_n,
	sdram_0_cke,
	sdram_0_cs_n,
	sdram_0_dq,
	sdram_0_dqm,
	sdram_0_ras_n,
	sdram_0_we_n,
	sdram_clk_clk,
	led_pio_export,
	button_pio_export,
	switch_pio_export,
	lcd_display_RS,
	lcd_display_RW,
	lcd_display_data,
	lcd_display_E,
	audio_i2c_SDAT,
	audio_i2c_SCLK,
	audio_out_ADCDAT,
	audio_out_ADCLRCK,
	audio_out_BCLK,
	audio_out_DACDAT,
	audio_out_DACLRCK,
	uart_rxd,
	uart_txd,
	spi_master_cs,
	spi_master_sclk,
	spi_master_mosi,
	spi_master_miso,
	spi_master_cd,
	spi_master_wp,
	dual_7_segment_segment_data,
	dual_7_segment_digit1,
	dual_7_segment_digit2,
	egm_interface_stimulus,
	egm_interface_response,
	egm_interface_egm_leds,
	stimulus_in_export,
	response_out_export);	

	input		clk_50_clk;
	input		reset_reset_n;
	output		audio_mclk_clk;
	output	[11:0]	sdram_0_addr;
	output	[1:0]	sdram_0_ba;
	output		sdram_0_cas_n;
	output		sdram_0_cke;
	output		sdram_0_cs_n;
	inout	[15:0]	sdram_0_dq;
	output	[1:0]	sdram_0_dqm;
	output		sdram_0_ras_n;
	output		sdram_0_we_n;
	output		sdram_clk_clk;
	output	[3:0]	led_pio_export;
	input	[3:0]	button_pio_export;
	input	[7:0]	switch_pio_export;
	output		lcd_display_RS;
	output		lcd_display_RW;
	inout	[7:0]	lcd_display_data;
	output		lcd_display_E;
	inout		audio_i2c_SDAT;
	output		audio_i2c_SCLK;
	input		audio_out_ADCDAT;
	input		audio_out_ADCLRCK;
	input		audio_out_BCLK;
	output		audio_out_DACDAT;
	input		audio_out_DACLRCK;
	input		uart_rxd;
	output		uart_txd;
	output		spi_master_cs;
	output		spi_master_sclk;
	output		spi_master_mosi;
	input		spi_master_miso;
	input		spi_master_cd;
	input		spi_master_wp;
	output	[7:0]	dual_7_segment_segment_data;
	output		dual_7_segment_digit1;
	output		dual_7_segment_digit2;
	output		egm_interface_stimulus;
	input		egm_interface_response;
	output	[3:0]	egm_interface_egm_leds;
	input		stimulus_in_export;
	output		response_out_export;
endmodule
