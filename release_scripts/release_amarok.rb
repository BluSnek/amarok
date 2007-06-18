#!/usr/bin/env ruby
#
# Ruby script for generating Amarok tarball releases from KDE SVN
#
# (c) 2005 Mark Kretschmann <markey@web.de>
# Some parts of this code taken from cvs2dist
# License: GNU General Public License V2


# Red Cards (removes translation)
doc_redcards = ""
po_redcards = ""

tag = nil
useStableBranch=false

# Ask whether using branch or trunk
location = `kdialog --combobox "Select checkout's place:" "Trunk" "Stable" "Tag"`.chomp()
if location == "Tag"
    tag = `kdialog --inputbox "Enter tag name: "`.chomp()
elsif location == "Stable"
    useStableBranch=true
end

# Ask user for targeted application version
if tag and not tag.empty?()
    version = tag
else
    version  = `kdialog --inputbox "Enter Amarok version: "`.chomp()
end
user = `kdialog --inputbox "Your SVN user:"`.chomp()
protocol = `kdialog --radiolist "Do you use https or svn+ssh?" https https 0 "svn+ssh" "svn+ssh" 1`.chomp()

name     = "amarok"
folder   = "amarok-#{version}"
do_l10n  = true


# Prevent using unsermake
oldmake = ENV["UNSERMAKE"]
ENV["UNSERMAKE"] = "no"

# Remove old folder, if exists
`rm -rf #{folder} 2> /dev/null`
`rm -rf #{folder}.tar.bz2 2> /dev/null`

Dir.mkdir( folder )
Dir.chdir( folder )
branch="trunk"

if useStableBranch
    branch="branches/stable/"
    `svn co -N #{protocol}://#{user}@svn.kde.org/home/kde/#{branch}/extragear/multimedia/`
    Dir.chdir( "multimedia" )
elsif not tag.empty?()
    do_l10n = false
    `svn co -N #{protocol}://#{user}@svn.kde.org/home/kde/tags/amarok/#{tag}/multimedia`
    Dir.chdir( "multimedia" )
else
    `svn co -N #{protocol}://#{user}@svn.kde.org/home/kde/trunk/extragear/multimedia`
     Dir.chdir( "multimedia" )
end

`svn up amarok`
`svn up -N doc`
`svn up doc/amarok`
`svn co #{protocol}://#{user}@svn.kde.org/home/kde/branches/KDE/3.5/kde-common/admin`


if do_l10n
    puts "\n"
    puts "**** l10n ****"
    puts "\n"

    i18nlangs = `svn cat #{protocol}://#{user}@svn.kde.org/home/kde/#{branch}/l10n/subdirs`
    Dir.mkdir( "l10n" )
    Dir.chdir( "l10n" )

    # docs
    for lang in i18nlangs
        lang.chomp!()
        `rm -Rf ../doc/#{lang}`
        `rm -rf amarok`
        docdirname = "l10n/#{lang}/docs/extragear-multimedia/amarok"
        `svn co -q #{protocol}://#{user}@svn.kde.org/home/kde/#{branch}/#{docdirname} > /dev/null 2>&1`
        next unless FileTest.exists?( "amarok" )
        print "Copying #{lang}'s #{name} documentation over..  "
        `cp -R amarok/ ../doc/#{lang}`

        # we don't want KDE_DOCS = AUTO, cause that makes the
        # build system assume that the name of the app is the
        # same as the name of the dir the Makefile.am is in.
        # Instead, we explicitly pass the name..
        makefile = File.new( "../doc/#{lang}/Makefile.am", File::CREAT | File::RDWR | File::TRUNC )
        makefile << "KDE_LANG = #{lang}\n"
        makefile << "KDE_DOCS = #{name}\n"
        makefile.close()

        puts( "done.\n" )
    end

    # red cards for documentation
    for redcard in doc_redcards
        `rm -rf #{redcard}`
        print "\n Removed #{redcard} due to broken translation in last release \n"
    end

    Dir.chdir( ".." ) # multimedia
    puts "\n"

    $subdirs = false
    Dir.mkdir( "po" )

    for lang in i18nlangs
        lang.chomp!()
        pofilename = "l10n/#{lang}/messages/extragear-multimedia/amarok.po"
        `svn cat #{protocol}://#{user}@svn.kde.org/home/kde/#{branch}/#{pofilename} 2> /dev/null | tee l10n/amarok.po`
        next if FileTest.size( "l10n/amarok.po" ) == 0

        dest = "po/#{lang}"
        Dir.mkdir( dest )
        print "Copying #{lang}'s #{name}.po over ..  "
        `mv l10n/amarok.po #{dest}`
        puts( "done.\n" )

        makefile = File.new( "#{dest}/Makefile.am", File::CREAT | File::RDWR | File::TRUNC )
        makefile << "KDE_LANG = #{lang}\n"
        makefile << "SUBDIRS  = $(AUTODIRS)\n"
        makefile << "POFILES  = AUTO\n"
        makefile.close()

        $subdirs = true
    end

    # red cards for translations
    for redcard in po_redcards
        `rm -rf #{redcard}`
        print "\n Removed #{redcard} due to broken translation in last release \n"
    end

    if $subdirs
        makefile = File.new( "po/Makefile.am", File::CREAT | File::RDWR | File::TRUNC )
        makefile << "SUBDIRS = $(AUTODIRS)\n"
        makefile.close()
        # Remove xx language
        `rm -rf po/xx`
    else
        `rm -rf po`
    end

    `rm -rf l10n`
end

puts "\n"


# Remove SVN data folder
`find -name ".svn" | xargs rm -rf`

if useStableBranch
    `svn co -N #{protocol}://#{user}@svn.kde.org/home/kde/trunk/extragear/multimedia`
    `mv multimedia/* .`
    `rmdir multimedia`
end

Dir.chdir( "amarok" )

# Move some important files to the root folder
`mv AUTHORS ..`
`mv ChangeLog ..`
`mv COPYING ..`
`mv INSTALL ..`
`mv README ..`
`mv TODO ..`

# This stuff doesn't belong in the tarball
`rm -rf src/scripts/rbot`
`rm -rf release_scripts`
`rm -rf src/engine/gst10` #Removed for now
`rm -rf autopackage`
`rm -rf supplementary_scripts`

Dir.chdir( "src" )

# Exchange APP_VERSION string with targeted version
file = File.new( "amarok.h", File::RDWR )
str = file.read()
file.rewind()
file.truncate( 0 )
str.sub!( /APP_VERSION \".*\"/, "APP_VERSION \"#{version}\"" )
file << str
file.close()


Dir.chdir( ".." ) # amarok
Dir.chdir( ".." ) # multimedia
puts( "\n" )

`find | xargs touch`
puts "**** Generating Makefiles..  "
`make -f Makefile.cvs`
puts "done.\n"

`rm -rf autom4te.cache`
`rm stamp-h.in`


puts "**** Compressing..  "
`mv * ..`
Dir.chdir( ".." ) # Amarok-foo
`rm -rf multimedia`
Dir.chdir( ".." ) # root folder
`tar -cf #{folder}.tar #{folder}`
`bzip2 #{folder}.tar`
`rm -rf #{folder}`
puts "done.\n"


ENV["UNSERMAKE"] = oldmake


puts "\n"
puts "====================================================="
puts "Congratulations :) Amarok #{version} tarball generated.\n"
puts "Now follow the steps in the RELEASE_HOWTO, from\n"
puts "SECTION 3 onwards.\n"
puts "\n"
puts "Then drink a few pints and have some fun on #amarok\n"
puts "\n"
puts "MD5 checksum: " + `md5sum #{folder}.tar.bz2`
puts "\n"
puts "\n"


