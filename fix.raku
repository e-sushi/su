my @stack = ".".IO;
while @stack {
    for @stack.pop.dir -> $path {
        @stack.push: $path and next if $path.d;
        # if $path.IO.extension ~~ 'h' | 'cpp' {
        #     $path.unlink;
        # } elsif $path.IO.extension ~~ 'backup' {
        #     $path.rename: $path ~~ /.*?<?before \.backup>/;
        # }

        process($path) if $path.IO.extension ~~ 'h' | 'cpp';
    }
}

sub process($path) {
    my $content = $path.slurp;
    # while $content ~~ m/<!after \/\/.*?>DString<!before [\*|\S|\s*[\{|\[]]>/ {
    #     $content = $/.replace-with("DString*");
    # }

    while $content ~~ m/DString<?before \s*\n\s*[name|dump]>/ {
        $content = $/.replace-with("DString*");
    }

    while $content ~~ /dstring\:\:init/ {
        $content = $/.replace-with("DString::create");
    }

    while $content ~~ /dstring\:\:append\((\w+)\,\s*(.*?)\)\;/ {
        $content = $/.replace-with("$0\->append($1);");
        # "{.caps[0].kv[1]}\->append({.caps[1].kv[1]})".say for $_;
    }

    while $content ~~ /dstring\:\:deinit\((.*?)\)/ {
        $content = $/.replace-with("$0\->deref($1)");
    } 

    while $content ~~ /DString\&/ {
        $content = $/.replace-with("DString*");
    }

    # say $content;

    # $path.copy: "$path.backup";
    $path.spurt: $content;
}