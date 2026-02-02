#!/usr/bin/env perl
# parse_profile.pl — Extract profile JSON from Guage stderr and output TSV
# Usage: GUAGE_PROFILE=1 ./guage bench.scm 2>&1 | perl parse_profile.pl [guage_time_s]
#
# Outputs tab-separated values for shell consumption:
#   eval_steps lambda_calls builtin_calls tail_calls env_lookups env_steps
#   prim_lookups prim_steps cell_allocs cell_frees retains releases
#   avg_env_depth avg_prim_depth ns_per_eval ns_per_lambda allocs_per_lambda
#   env_steps_per_lambda prim_steps_per_builtin

use strict;
use warnings;

my $guage_time = $ARGV[0] || 0;  # seconds

my $line;
while (<STDIN>) {
    if (/"t":"profile"/) {
        $line = $_;
        last;
    }
}

unless ($line) {
    # No profile data found — output zeros
    print join("\t", (0) x 17) . "\n";
    exit 0;
}

# Extract fields from JSON
my %d;
while ($line =~ /"(\w+)":(\d+(?:\.\d+)?)/g) {
    $d{$1} = $2;
}

my @raw = map { $d{$_} || 0 } qw(
    eval_steps lambda_calls builtin_calls tail_calls
    env_lookups env_steps prim_lookups prim_steps
    cell_allocs cell_frees retains releases
);

my $avg_env  = $d{avg_env_depth}  || 0;
my $avg_prim = $d{avg_prim_depth} || 0;

# Derived metrics
my $ns_per_eval    = ($d{eval_steps}    > 0 && $guage_time > 0) ? ($guage_time * 1e9) / $d{eval_steps}    : 0;
my $ns_per_lambda  = ($d{lambda_calls}  > 0 && $guage_time > 0) ? ($guage_time * 1e9) / $d{lambda_calls}  : 0;
my $allocs_per_lam = ($d{lambda_calls}  > 0)                    ? $d{cell_allocs}      / $d{lambda_calls}  : 0;
my $env_per_lam    = ($d{lambda_calls}  > 0)                    ? $d{env_steps}         / $d{lambda_calls}  : 0;
my $prim_per_bi    = ($d{builtin_calls} > 0)                    ? $d{prim_steps}        / $d{builtin_calls} : 0;

printf "%s\t%.1f\t%.1f\t%.1f\t%.1f\t%.1f\t%.1f\t%.1f\n",
    join("\t", @raw),
    $avg_env, $avg_prim,
    $ns_per_eval, $ns_per_lambda, $allocs_per_lam, $env_per_lam, $prim_per_bi;
