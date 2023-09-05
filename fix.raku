my @stack = ".".IO;
while @stack {
    for @stack.pop.dir -> $path {
        @stack.push: $path and next if $path.d;
        if $path.IO.extension ~~ 'h' | 'cpp' {
            $path.unlink;
        } elsif $path.IO.extension ~~ 'backup' {
            say $path ~~ /.*?<?before \.backup>/;
        }

        # process($path) if $path.IO.extension ~~ 'backup' # 'h' | 'cpp'
    }
}

sub process($path) {
    my $content = $path.slurp;
    while $content ~~ m/DString<!before \*>/ {
        $content = $/.replace-with("DString*");
    }

    while $content ~~ /dstring\:\:init/ {
        $content = $/.replace-with("DString::create");
    }

    while $content ~~ /dstring\:\:append\((\w+)\,\s*(.*?)\)\;/ {
        $content = $/.replace-with("$0\->append($1);");
        # "{.caps[0].kv[1]}\->append({.caps[1].kv[1]})".say for $_;
    }

    say $content;

    $path.copy: "$path.backup";
    $path.spurt: $content;
}