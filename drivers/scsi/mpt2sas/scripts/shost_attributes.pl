#!/usr/bin/perl

#main
my($path) = "/sys/class/scsi_host";
my(@line);
my($shost);
my($proc_name);

chdir($path);
@line = `ls -1`;

foreach (@line) {
	chomp;
	$shost = join("/", $path, $_);
	$proc_name = `cat $shost/proc_name`;
	chomp $proc_name;
	&print_shost($shost, $_) if ($proc_name eq 'mptspi');
	&print_shost($shost, $_) if ($proc_name eq 'mptsas');
	&print_shost($shost, $_) if ($proc_name eq 'mpt2sas');
}

# subroutine: print_shost
sub print_shost
{
	my($shost_path, $shost_name) = @_;
	my($attribute);
	my(@line);

	chdir($shost_path);
	print $shost_name, "\n";
	@line = `ls -1`;
	foreach $attribute (@line) {
		chomp $attribute;
		&print_attribute($attribute) if (-f $attribute);
	}
}

#subroutine: print_attribute
sub print_attribute
{
	my($attribute_name) = @_;
	my($contents);

	return if ($attribute_name eq 'scan');
	return if ($attribute_name eq 'uevent');

	$contents = `cat $attribute_name`;
	print "\t$attribute_name = $contents";
}

