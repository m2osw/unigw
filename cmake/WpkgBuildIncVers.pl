#!/usr/bin/perl -w
#
my $version = <STDIN>;

if( $version =~ m/^(\d*).(\d+).(\d+)$/ )
{
	print "$1.$2.$3.1";
}
elsif( $version =~ m/^(\d*).(\d+).(\d+).(\d+)$/ )
{
	my $inc = $4+1;
	print "$1.$2.$3.$inc";
}
