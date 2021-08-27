#!/usr/bin/perl

# Save original .config
open cfg_file, ".config" or die "\n AMP Error: Can't open .config file. Aborting\n";
open boot_file, "arch/arm/mach-armadaxp/Makefile.boot" or die "\n AMP Error: Can't open arch/arm/mach-armadaxp/Makefile.boot file. Aborting\n";

$curr_base = $base[0] = $base[1] = $curr_port = $port[0] =  $port[1] = -1;
$load_addr = $pars_addr = $ramd_addr = -1;


$argc = @ARGV;
$out_dir = "./arch/arm/boot";


if($argc > 0)
{
	if(-d $ARGV[0])
	{
		$out_dir = $ARGV[0];
		#Clip trailing slash if exists
		$out_dir =~ s/\/$//;
	}
	else
	{
		print "\nAMP: Output directory $ARGV[0] doesnt exist. Using defualt $out_dir\n";
	}
}


while( $line = <cfg_file>)
{
	if($line =~ m/CONFIG_MV_DRAM_BASE=(\S*)/)   {$curr_base = $1;}
	if($line =~ m/CONFIG_MV_DRAM_BASE_G0=(\S*)/){$base[0] = $1;}
	if($line =~ m/CONFIG_MV_DRAM_BASE_G1=(\S*)/){$base[1] = $1;}
	if($line =~ m/CONFIG_MV_UART_PORT=(\d*)/)     {$curr_port = $1;}
	if($line =~ m/CONFIG_MV_UART_PORT_G0=(\d*)/)  {$port[0] = $1;}
	if($line =~ m/CONFIG_MV_UART_PORT_G1=(\d*)/)  {$port[1] = $1;}
}
while( $line = <boot_file>)
{
	if($line =~ m/zreladdr-y\s:= (0x[0-9a-fA-F]*)$/){$load_addr = $1;}
	if($line =~ m/params_phys-y\s:= (0x[0-9a-fA-F]*)$/){$pars_addr = $1;}
	if($line =~ m/initrd_phys-y\s:= (0x[0-9a-fA-F]*)$/){$ramd_addr = $1;}
}


if($curr_base == -1 or
   $base[0]   == -1 or
   $base[1]   == -1 or
   $curr_port == -1 or
   $port[0]   == -1 or
   $port[1]   == -1 or
   $load_addr == -1 or
   $pars_addr == -1 or
   $ramd_addr == -1  )
{
	print "AMP Error: Cant find all CONFIG values in .config. Did you set
CONFIG_MV_AMP_ENABLE ?\n";

	print "curr_base = $curr_base\n";
	print "base[0]   = $base[0]\n";
	print "base[1]   = $base[1]\n";
	print "curr_port = $curr_port\n";
	print "port[0]   = $port[0]\n";
	print "port[1]   = $port[1]\n";
	print "load_addr = $load_addr\n";
	print "pars_addr = $pars_addr\n";
	print "ramd_addr = $ramd_addr\n";

	goto END;
}

# To speed up compilation, start from the last compiled group
$g_id = 0;
$add  = 1;

if($curr_base eq $ base[1])
{
	$g_id =  1;
	$add  = -1;
}

#####	compile both image #######
for (; ($g_id < 2) and ($g_id >= 0); $g_id += $add)
{
	print ("\nAMP: Compiling image $g_id\n");

	# Calculate new addresses based on new base address
	$new_load = hex($base[$g_id]) + hex($load_addr);
	$new_pars = hex($base[$g_id]) + hex($pars_addr);
	$new_ramd   = hex($base[$g_id]) + hex($ramd_addr);

	$new_load = sprintf("0x%x", $new_load);
	$new_pars = sprintf("0x%x", $new_pars);
	$new_ramd = sprintf("0x%x", $new_ramd);

	# Modify .config
	system("perl -p -i -e \"s/CONFIG_MV_DRAM_BASE=.*/CONFIG_MV_DRAM_BASE=$base[$g_id]/\" .config");
	system("perl -p -i -e \"s/CONFIG_MV_UART_PORT=.*/CONFIG_MV_UART_PORT=$port[$g_id]/\" .config");
	system("perl -p -i -e \"s/CONFIG_MV_ZREL_ADDR=.*/CONFIG_MV_ZREL_ADDR=$new_load/\" .config");
	system("perl -p -i -e \"s/CONFIG_MV_PARAM_PHYS=.*/CONFIG_MV_PARAM_PHYS=$new_pars/\" .config");
	system("perl -p -i -e \"s/CONFIG_MV_INITRD_PHYS=.*/CONFIG_MV_INITRD_PHYS=$new_ramd/\" .config");

	#Compile
	$fail = system("make uImage -j4");

	if($fail)
	{
		print "\nAMP Error: Failed to build Image $g_id. Exiting\n";
		goto END;
	}

	system("cp ./arch/arm/boot/uImage $out_dir/uImage_g$g_id");
	print "\nAMP: Image $g_id ready at $out_dir/uImage_g$g_id\n";
}

END:

close cfg_file;
close boot_file;
