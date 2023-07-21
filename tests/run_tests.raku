use Terminal::ANSIColor;

my $longest_path = 0;
my @queue = [];
my @path_stack = '.'.IO;
while @path_stack {
    for @path_stack.pop.dir -> $path {
        next if $path eq 'run_tests.raku' or $path.starts-with: '_';
        @path_stack.push: $path and next if $path.d;
        @queue.push: ("$path", open $path, :r);
        $longest_path max= "$path".chars;
    }
}

$longest_path += 2;

for @queue.kv -> $num, $path {
    say "testing {$path[0]}";
    test $path[1];

    # if 1.rand.round {
    #     say color('red'), "." x $longest_path - $path[0].chars+1, "failed!", color('reset');
    # } else {
    #     say color('green'), "." x $longest_path - $path[0].chars, "success!", color('reset');
    # }
}

sub test($fh) {
    my $proc = run '../build/debug/amu', $fh.path.Str;
    $proc.so;
    # say $proc.out.slurp: :close;
    # my $diags = 'out'.IO.slurp: :bin;
    # my $string_header_len = $diags[0];
    # return True unless $string_header_len;
}