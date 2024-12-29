package main

import (
	"fmt"
	"os"
	"strings"

	"github.com/darklab8/fl-data-discovery/autopatcher"
	"github.com/darklab8/go-utils/utils/utils_types"
)

func main() {
	os.Chdir("./freelancer_folder")
	println(os.Getwd())
	current_directory, err := os.Getwd()
	if err != nil {
		panic(err)
	}

	// var patch autopatcher.Patch = autopatcher.Patch{}
	freelancer_folder := autopatcher.ScanCaseInsensitiveFS(".")
	freelancer_folder_path := utils_types.FilePath(current_directory)
	patch_folder_path := freelancer_folder_path.Dir().Join("Freelancer")
	patch_folder := autopatcher.ScanCaseInsensitiveFS(patch_folder_path.ToString())

	for _, file := range patch_folder.Files {

		content, err := os.ReadFile(file.GetPath())
		if err != nil {
			panic(fmt.Sprintln("failed to read file", err))
		}

		relative_patch_filepath := file.GetRelPathTo(patch_folder_path.ToString())

		if strings.Contains(relative_patch_filepath, ".gitignore") {
			continue
		}

		if freelancer_path, file_exists := freelancer_folder.LowerMapFiles[strings.ToLower(relative_patch_filepath)]; file_exists {
			os.Remove(freelancer_path.GetPath())
		}

		relative_patch_filepath = autopatcher.AdjustFoldersInPath(relative_patch_filepath, freelancer_folder)

		autopatcher.WriteToFile(relative_patch_filepath, content)
	}
}
