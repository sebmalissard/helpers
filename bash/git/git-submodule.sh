#!/bin/bash

list=/home/seb/Workspace/submodule/submodule_list
server=3

git-submodule-update-url()
{
    local url_list=${1}
    local git_dir=${2:-.}
    
    usage()
    {
        echo "git-submodule-update-url <url_list> [git_path]"
        echo "      url_list: File with the following format:"
        echo "               submodule.[sub1].url [https://git/sub1.git]"
        echo "               submodule.[sub2].url [https://git/sub2.git]"
        echo "      git_path (optional): Run as if git was started in git_path instead of the current working directory."
    }
    
    [ ! -d "${git_dir}/.git" ] && echo "ERROR: Invalid git repository \"${git_dir}\"" && usage && return 1
    [ ! -f "${url_list}" ]     && echo "ERROR: File \"${url_list}\" doesn't exist" && usage && return 1
    
    while read -r submodule; do
        name=$(echo ${submodule} | awk '{print $1}')
        old_url=$(echo ${submodule} | awk '{print $2}')
        new_url=$(grep ${name} ${url_list} | awk '{print $2}')
        [ -z ${new_url} ] && echo "INFO: No new URL for ${name}, current url: ${old_url}" && continue
        [ "${old_url}" == "${new_url}" ] && echo "INFO: Same URL for ${name}: ${old_url}" && continue
        echo "INFO: Change ${name} from ${old_url} to ${new_url}"
        git -C ${git_dir} config ${name} ${new_url}    
    done < <(git -C ${git_dir} config --file .gitmodules --get-regexp url)
}

# Same as "git submodule update --init --recursive" but change the url of the submodule if required
git_submodule_update()
{
    local gitdir=${1:-.}
    echo -e "gitdir=$gitdir,      count=$2\n"
    
    while read -r gitmodule; do
        echo gitmodule=$gitmodule
        #git_submodule_change_url $(dirname ${gitmodule})
        echo "Update submodule:::::::::$(dirname ${gitmodule})"
        git -C $(dirname ${gitmodule}) submodule update --init
        while read -r submodule_path; do
            echo ">>>>>>>>>>>>>>>>>>"
            echo submodule_path=$submodule_path
            git_submodule_update ${gitdir}/${submodule_path} $(($2+1))
            echo "<<<<<<<<<<<<<<<<<<"
        done < <(git -C ${gitdir} config --file=.gitmodules --get-regexp path | awk '{print $2}')
    done < <(find ${gitdir} -name .gitmodules)

}

git_submodule_update

#unset -f git_submodule_change_url
#unset -f git_submodule_update
