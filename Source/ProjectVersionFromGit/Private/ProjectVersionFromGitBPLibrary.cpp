// Copyright 2020 Andrew Bindraw. All Rights Reserved.

#include "ProjectVersionFromGitBPLibrary.h"
#include "ProjectVersionFromGit.h"

FText UProjectVersionFromGitBPLibrary::ProjectVersion 			= FText::GetEmpty();
FText UProjectVersionFromGitBPLibrary::ProjectVersionFormatAll 	= FText::GetEmpty();
FText UProjectVersionFromGitBPLibrary::BranchName 				= FText::GetEmpty();
FText UProjectVersionFromGitBPLibrary::CommitHash 				= FText::GetEmpty();
FText UProjectVersionFromGitBPLibrary::DateTimeBuild 			= FText::GetEmpty();

int32 UProjectVersionFromGitBPLibrary::Major = 0;
int32 UProjectVersionFromGitBPLibrary::Minor = 0;
int32 UProjectVersionFromGitBPLibrary::Patch = 0;

FString UProjectVersionFromGitBPLibrary::SectionName	= FString(TEXT("VersionInfo"));
FString UProjectVersionFromGitBPLibrary::VersionFileIni	= FString(TEXT("Version.ini"));

FString UProjectVersionFromGitBPLibrary::GitStdOutput = FString(TEXT(""));

DEFINE_LOG_CATEGORY(ProjectVersionFromGit)

UProjectVersionFromGitBPLibrary::UProjectVersionFromGitBPLibrary(const FObjectInitializer& ObjectInitializer)
: Super(ObjectInitializer)
{
	GetProjectVersion();
}

bool UProjectVersionFromGitBPLibrary::ExecProcess(const TCHAR* URL, const TCHAR* Params, int32* OutReturnCode, FString* OutStdOut, FString* OutStdErr, const TCHAR* OptionalWorkingDirectory)
{
	if (!GEngine->IsEditor())
	{
		return false;
	}

#if ENGINE_MINOR_VERSION >= 23
	return FPlatformProcess::ExecProcess(URL, Params, OutReturnCode, OutStdOut, OutStdErr, OptionalWorkingDirectory);
#else
	const FString command = FString(URL) + FString(TEXT(" -C ")) + FString(OptionalWorkingDirectory);
	return FPlatformProcess::ExecProcess(*command, Params, OutReturnCode, OutStdOut, OutStdErr);
#endif
}

FText UProjectVersionFromGitBPLibrary::GetProjectVersion()
{
	static const FString VersionFileIniPath = FString::Printf(TEXT("%s%s"), *FPaths::SourceConfigDir(), *VersionFileIni);

	if (GEngine->IsEditor())
	{
		FString OutStdOut;
		FString OutStdErr;
		int32 OutReturnCode;
		static const FString OptionalWorkingDirectory = FPaths::ConvertRelativePathToFull(FPaths::ProjectDir());

		FString TagNameArg;

		ExecProcess(TEXT("git"), TEXT("rev-list --tags --max-count=1"), &OutReturnCode, &OutStdOut, &OutStdErr, *OptionalWorkingDirectory);

		TagNameArg = FString(TEXT("describe --tags ")) + OutStdOut.TrimStartAndEnd();
		ExecProcess(TEXT("git"), *TagNameArg, &OutReturnCode, &OutStdOut, &OutStdErr, *OptionalWorkingDirectory);
		//UE_LOG(ProjectVersionFromGit, Log, TEXT("-------- Git tag: %s"), *OutStdOut);

		const FRegexPattern myPattern(TEXT("([0-9]\\.[0-9]\\.[0-9])+"));
		FRegexMatcher myMatcher(myPattern, OutStdOut);

		if (myMatcher.FindNext())
		{
			int32 beginPos = myMatcher.GetMatchBeginning();
			int32 endPos = myMatcher.GetMatchEnding();
			//UE_LOG(ProjectVersionFromGit, Log, TEXT("Regex git tag pos: %i %i"), beginPos, endPos);
			OutStdOut = OutStdOut.Mid(beginPos, endPos - beginPos);
		}
		UE_LOG(ProjectVersionFromGit, Log, TEXT("-------- Git tag: %s"), *OutStdOut);

		if (OutStdOut.IsEmpty())
		{
			ProjectVersion = FText::FromString(TEXT("0.0.0"));
			Major = Minor = Patch = 0;
		}
		else
		{
			OutStdOut = OutStdOut.TrimStartAndEnd();
			TArray<FString> OutArrayParse;
			OutStdOut.ParseIntoArrayWS(OutArrayParse, TEXT("."));

			if (OutArrayParse.Num() == 3)
			{
				FDefaultValueHelper::ParseInt(OutArrayParse[0], Major);
				FDefaultValueHelper::ParseInt(OutArrayParse[1], Minor);
				FDefaultValueHelper::ParseInt(OutArrayParse[2], Patch);

				ProjectVersion = FText::FromString(OutStdOut);
			}
			else
			{
				ProjectVersion = FText::FromString(TEXT("0.0.0"));
				Major = Minor = Patch = 0;
			}
		}

		DateTimeBuild = FText::FromString(FDateTime::UtcNow().ToString() + TEXT("-UTC"));

		GetProjectVersionBranchName();
		GetProjectVersionCommitHash();

		ProjectVersionFormatAll = FText::Format(FText::FromString(TEXT("{0}-{1}-{2}-{3}")), ProjectVersion, BranchName, CommitHash, DateTimeBuild);

		if (!GitStdOutput.IsEmpty())
		{
			UE_LOG(ProjectVersionFromGit, Warning, TEXT("-------- Git status --short: %s"), *GitStdOutput);
		}

		GConfig->SetText(
			*SectionName,
			TEXT("ProjectVersion"),
			ProjectVersion,
			VersionFileIniPath
		);

		GConfig->SetInt(
			*SectionName,
			TEXT("Major"),
			Major,
			VersionFileIniPath
		);

		GConfig->SetInt(
			*SectionName,
			TEXT("Minor"),
			Minor,
			VersionFileIniPath
		);

		GConfig->SetInt(
			*SectionName,
			TEXT("Patch"),
			Patch,
			VersionFileIniPath
		);

		GConfig->SetText(
			*SectionName,
			TEXT("BranchName"),
			BranchName,
			VersionFileIniPath
		);

		GConfig->SetText(
			*SectionName,
			TEXT("CommitHash"),
			CommitHash,
			VersionFileIniPath
		);

		GConfig->SetText(
			*SectionName,
			TEXT("DateTimeBuild"),
			DateTimeBuild,
			VersionFileIniPath
		);

		GConfig->SetText(
			*SectionName,
			TEXT("ProjectVersionFormatAll"),
			ProjectVersionFormatAll,
			VersionFileIniPath
		);
	}
	else
	{
		GConfig->GetText(
			*SectionName,
			TEXT("ProjectVersion"),
			ProjectVersion,
			VersionFileIniPath
		);

		GConfig->GetInt(
			*SectionName,
			TEXT("Major"),
			Major,
			VersionFileIniPath
		);

		GConfig->GetInt(
			*SectionName,
			TEXT("Minor"),
			Minor,
			VersionFileIniPath
		);

		GConfig->GetInt(
			*SectionName,
			TEXT("Patch"),
			Patch,
			VersionFileIniPath
		);

		GConfig->GetText(
			*SectionName,
			TEXT("BranchName"),
			BranchName,
			VersionFileIniPath
		);

		GConfig->GetText(
			*SectionName,
			TEXT("CommitHash"),
			CommitHash,
			VersionFileIniPath
		);

		GConfig->GetText(
			*SectionName,
			TEXT("DateTimeBuild"),
			DateTimeBuild,
			VersionFileIniPath
		);

		GConfig->GetText(
			*SectionName,
			TEXT("ProjectVersionFormatAll"),
			ProjectVersionFormatAll,
			VersionFileIniPath
		);
	}

	UE_LOG(ProjectVersionFromGit, Log, TEXT("-------- ProjectVersion: %s"), *ProjectVersion.ToString());
	UE_LOG(ProjectVersionFromGit, Log, TEXT("-------- Major: %d"), Major);
	UE_LOG(ProjectVersionFromGit, Log, TEXT("-------- Minor: %d"), Minor);
	UE_LOG(ProjectVersionFromGit, Log, TEXT("-------- Patch: %d"), Patch);
	UE_LOG(ProjectVersionFromGit, Log, TEXT("-------- BranchName: %s"), *BranchName.ToString());
	UE_LOG(ProjectVersionFromGit, Log, TEXT("-------- CommitHash: %s"), *CommitHash.ToString());
	UE_LOG(ProjectVersionFromGit, Log, TEXT("-------- DateTimeBuild: %s"), *DateTimeBuild.ToString());
	UE_LOG(ProjectVersionFromGit, Log, TEXT("-------- ProjectVersionFormatAll: %s"), *ProjectVersionFormatAll.ToString());
	//UE_LOG(ProjectVersionFromGit, Log, TEXT("-------- GGameIni: %s"), *GGameIni);
	//UE_LOG(ProjectVersionFromGit, Log, TEXT("-------- VersionFileIniPath: %s"), *VersionFileIniPath);

	return ProjectVersion;
}

FText UProjectVersionFromGitBPLibrary::GetProjectVersionBranchName()
{
	if (GEngine->IsEditor())
	{
		FString OutStdOut;
		FString OutStdErr;
		int32 OutReturnCode;
		static const FString OptionalWorkingDirectory = FPaths::ConvertRelativePathToFull(FPaths::ProjectDir());

		if (GEngine->IsEditor())
		{
			ExecProcess(TEXT("git"), TEXT("symbolic-ref --short HEAD"), &OutReturnCode, &OutStdOut, &OutStdErr, *OptionalWorkingDirectory);
			if (OutStdOut.IsEmpty())
			{
				BranchName = FText::FromString(TEXT("unknown"));
			}
			else
			{
				BranchName = FText::FromString(OutStdOut.TrimStartAndEnd());
			}
		}
	}

	return BranchName;
}

FText UProjectVersionFromGitBPLibrary::GetProjectVersionCommitHash()
{
	if (GEngine->IsEditor())
	{
		FString OutStdOut;
		FString OutStdErr;
		int32 OutReturnCode;
		static const FString OptionalWorkingDirectory = FPaths::ConvertRelativePathToFull(FPaths::ProjectDir());

		ExecProcess(TEXT("git"), TEXT("status --short"), &OutReturnCode, &OutStdOut, &OutStdErr, *OptionalWorkingDirectory);
		GitStdOutput = OutStdOut;

		if (OutStdOut.IsEmpty())
		{
			ExecProcess(TEXT("git"), TEXT("describe --always --abbrev=8"), &OutReturnCode, &OutStdOut, &OutStdErr, *OptionalWorkingDirectory);
			CommitHash = FText::FromString(OutStdOut.TrimStartAndEnd());
		}
		else
		{
			CommitHash = FText::FromString(TEXT("unknown"));
		}
	}

	return CommitHash;
}

int32 UProjectVersionFromGitBPLibrary::GetProjectVersionMajor()
{
	return Major;
}

int32 UProjectVersionFromGitBPLibrary::GetProjectVersionMinor()
{
	return Minor;
}

int32 UProjectVersionFromGitBPLibrary::GetProjectVersionPatch()
{
	return Patch;
}

FText UProjectVersionFromGitBPLibrary::GetProjectVersionDateTimeBuild()
{
	return DateTimeBuild;
}

FText UProjectVersionFromGitBPLibrary::GetProjectVersionFormatAll()
{
	return ProjectVersionFormatAll;
}

