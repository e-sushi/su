

my @path_stack = '..'.IO;
while @path_stack {
    for @path_stack.pop.dir -> $path {
        @path_stack.push: $path and next if $path.d and not $path.d.starts-with: '../_';
        my $fh = open $path;
        test($fh);
    }
}

sub test($fh) {
    say "testing {$fh.path}";

    my $run = run '../build/debug/amu', '--dump-diagnostics', '../temp/diag', $fh.path.Str, :out;
}