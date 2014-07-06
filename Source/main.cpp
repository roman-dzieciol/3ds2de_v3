#include "3ds2de.h"


const char RegistryPath[] = "Software\\Tackware\\3ds2de";


//===========================================================================
static void    Add3DSFileToModel( const char* FileName, cUnrealModel* Model );
static string  GetBaseName( int argc, char* argv[], int* CurArg );
static void    GetProjectDirectory();
static void    SetProjectDirectory();
static void    Usage();    // doesn't return!


//===========================================================================
string          gBaseName;
string          gProjectDirectory;
cUnrealModel    gModel;


//===========================================================================
int main( int argc, char* argv[] )
{
    bool ShowCopyright = true;

    // Parse options
    int CurArg;
    for( CurArg = 1; CurArg < argc; ++CurArg ) {
        if( *argv[ CurArg ] == '-' ) {

            if( !stricmp( argv[ CurArg ], "-setproj" ) ) {
                SetProjectDirectory();
                exit( 0 );

            } else if (!stricmp( argv[ CurArg ], "-c" ) ) {
                ShowCopyright = false;

            } else {
                Usage();
            }
        } else {
            break;  // done with options
        }
    }

    if( ShowCopyright ) {
		printf("\n3DS to Deus Ex mesh converter, 3ds2de V%s\n", Version );
        printf("Based on 3ds2unr, Copyright (C) 1998 Legend Entertainment Co.\n");
        printf("Deus Ex modifications by Steve Tack\n");
        printf("Added Jim's V2 material flag syntax and two new flags - Switch`\n");
    }

    GetProjectDirectory();

    if( argc - CurArg < 1 )
        Usage();

    // Parse BaseName
    gBaseName = GetBaseName( argc, argv, &CurArg );

    if( argc - CurArg < 1 )
        Usage();

    try {
        // Process 3DS files
        for( ; CurArg < argc; ++CurArg ) {

            WIN32_FIND_DATA FindData;
            HANDLE FindH = ::FindFirstFile( argv[ CurArg ], &FindData );

            if( FindH != INVALID_HANDLE_VALUE ) {

                Add3DSFileToModel( FindData.cFileName, &gModel );
                while( ::FindNextFile( FindH, &FindData ) ) {
                    Add3DSFileToModel( FindData.cFileName, &gModel );
                }

                if( ::GetLastError() != ERROR_NO_MORE_FILES )
                    throw cxFile3DS( "error accessing file" );

                ::FindClose( FindH );

            } else { 
                throw cxFile3DS( "can't find file" );
            }
        }

        // Create Unreal files
        if( gModel.GetNumPolygons() > 0 )
            gModel.Write( gProjectDirectory, gBaseName );
    }
    catch( const cxFile3DS& e ) {
        printf( "%s: %s\n", argv[ CurArg ], e.what() );
    }
    catch( ... ) {
        printf( "%s: got unknown exception\n", argv[ CurArg ] );
    }

    return 0;
}

//===========================================================================
static void Add3DSFileToModel( const char* FileName, cUnrealModel* Model )
{
    cFile3DS            CurFile( FileName );
    string              SeqName = FileName;

    SeqName.erase( SeqName.find_last_of("."), SeqName.size() );
    Model->NewSequence( SeqName.c_str(), CurFile.GetNumFrames() );

    for( int i = 0; i < CurFile.GetNumFrames(); ++i ) { 
        bool FoundBadCoord = false;
        
        cFile3DS::cXYZList::const_iterator CurVertex = CurFile.BeginXYZ( i );
        cFile3DS::cXYZList::const_iterator EndVertex = CurFile.EndXYZ( i );
    
        while( CurVertex != EndVertex ) {
            Model->AddVertex( CurVertex->X, CurVertex->Y, CurVertex->Z );

            if( !FoundBadCoord ) {
                if( CurVertex->X <= -128.0 || 128.0 <= CurVertex->X ||
                    CurVertex->Y <= -128.0 || 128.0 <= CurVertex->Y ||
                    CurVertex->Z <= -128.0 || 128.0 <= CurVertex->Z ) {
                    printf( "warning: %s: bad coordinate %f,%f,%f\n",
                            FileName,
                            CurVertex->X, CurVertex->Y, CurVertex->Z );
                    FoundBadCoord = true;
                }
            }

            ++CurVertex;
        }
    }

    // Only add the faces to the model once, for the first anim sequence
    if( Model->GetNumPolygons() == 0 ) {
        cFile3DS::cFaceList::const_iterator CurFace = CurFile.BeginFace();
        cFile3DS::cFaceList::const_iterator EndFace = CurFile.EndFace();

        while( CurFace != EndFace ) {
            cFile3DS::Face F = *CurFace;
            Model->AddPolygon( cUnrealPolygon( F.V0, F.V1, F.V2,
                                               F.Type, 
                                               F.V0U, F.V0V,
                                               F.V1U, F.V1V,
                                               F.V2U, F.V2V,
                                               F.TextureNum ) );
            ++CurFace;
        }

        for( int TexNum = 0; TexNum < 10; ++TexNum ) {
            string TexName = CurFile.GetTextureName( TexNum );
            if( !TexName.empty() )
                Model->AddTexture( TexNum, TexName );
        }
    }
}

//===========================================================================
static string GetBaseName( int argc, char* argv[], int* CurArg )
{
    char    DirStr[_MAX_DIR];
    char    DriveStr[_MAX_DRIVE];
    char    ExtStr[_MAX_EXT];
    char    NameStr[_MAX_FNAME];

    _splitpath( argv[ *CurArg ], DriveStr, DirStr, NameStr, ExtStr );

    int SpanLen = strcspn( argv[ *CurArg ], "?*" );
    bool HasWildcards =  SpanLen != strlen( argv[ *CurArg ] );

    // If the current arg has no funny stuff (drive/dir names, wildcards)
    // then assume it's a class name.
    if( !*DriveStr && !*DirStr && !*ExtStr && !HasWildcards ) {
        ++*CurArg;

    } else { // user didn't provide a class name.  Intentionally?

        // If multiple files or wildcards given, user probably just forgot
        if( argc - *CurArg > 1 || HasWildcards ) {
            if( HasWildcards ) // don't use '*' as default name!
                strcpy( NameStr, "Junk" );

            string NameStrCopy = NameStr;
            printf( "Class name [%s]? ", NameStr );
            fgets( NameStr, _MAX_FNAME, stdin );

            if( NameStr[ strlen( NameStr ) - 1 ] == '\n' )
                NameStr[ strlen( NameStr ) - 1 ] = '\0';

            if( !*NameStr ) // empty string? use default
                strcpy( NameStr, NameStrCopy.c_str() );
        }
        // else only one file name, no wildcards -- use it as base name
    }

    return NameStr;
}

//===========================================================================
static void GetProjectDirectory()
{
    char Buffer[MAX_PATH];

    cRegistry   RootReg( RegistryPath );
    RootReg.GetValue( "ProjDir", Buffer, MAX_PATH );

    // No registry set?
    if( !*Buffer ) {
        SetProjectDirectory();
        RootReg.GetValue( "ProjDir", Buffer, MAX_PATH );
        if( !*Buffer ) {
            printf( "No project directory found -- exiting\n" );
            exit( 0 );
        }
    }

    gProjectDirectory = Buffer;
}

//===========================================================================
static void SetProjectDirectory()
{
    char            Buffer[_MAX_PATH + 1];
    BROWSEINFO      bi;
    ::ZeroMemory( &bi, sizeof bi );
    bi.lpszTitle      = "Choose Project Directory for 3ds2de";
    bi.pszDisplayName = &Buffer[ 0 ];
    bi.ulFlags        = BIF_RETURNONLYFSDIRS;

    LPITEMIDLIST    pItemIDList;
    if( ( pItemIDList = ::SHBrowseForFolder( &bi ) ) != 0 )
        if( ::SHGetPathFromIDList( pItemIDList, Buffer ) ) {
            cRegistry RootReg( RegistryPath );
            RootReg.SetValue( "ProjDir", Buffer );

            printf( "Project directory is now `%s'\n", Buffer );

            string NewDirPath = Buffer;
            NewDirPath += "\\";
            NewDirPath += "Models";
            ::CreateDirectory( NewDirPath.c_str(), 0 );

            NewDirPath = Buffer;
            NewDirPath += "\\";
            NewDirPath += "Classes";
            ::CreateDirectory( NewDirPath.c_str(), 0 );
        }
}

//===========================================================================
static void Usage()
{
    puts( "usage: 3ds2de -setproj" );
    puts( "       3ds2de ClassName file1.3ds file2.3ds ..." );
    puts( "       3ds2de ClassName.3ds" );
    exit( 0 );
}
