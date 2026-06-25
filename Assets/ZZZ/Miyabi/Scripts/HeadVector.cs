using System.Collections;
using System.Collections.Generic;
using UnityEngine;

[ExecuteInEditMode]
public class NewBehaviourScript : MonoBehaviour
{
    //挂载的空组件容器
    //脚本外部指定
    public Transform HeadBoneTransform;
    public Transform HeadForwardTransform;
    public Transform HeadRightTransform;

    private Renderer[] allRanderers;
    //传出的值在Shader中的命名
    private int headCenterID = Shader.PropertyToID("_HeadCenter");
    private int headForwardID = Shader.PropertyToID("_HeadForward");
    private int headRightID = Shader.PropertyToID("_HeadRight");
    //脚本开启时也更新一次
#if UNITY_EDITOR
    void OnValidate()
    {
        Update();
    }
#endif

    // Update is called once per frame
    void Update()
    {
        //获取所有渲染组件
        if (allRanderers == null)
        {
            allRanderers = GetComponentsInChildren<Renderer>(true);
        }
        //遍历各组件
        for (int i = 0; i < allRanderers.Length; i++)
        {
            Renderer r = allRanderers[i];
            //遍历各组件材质
            foreach (Material mat in r.sharedMaterials)
            {
                if (mat.shader)
                {
                    //这里应该多次设置给不同的子材质，为了省事直接传的父材质
                    if (mat.shader.name == "Custom/ZZZ")
                    {
                        mat.SetVector(headCenterID, HeadBoneTransform.position);
                        mat.SetVector(headForwardID, HeadForwardTransform.position);
                        mat.SetVector(headRightID, HeadRightTransform.position);

                    }
                }
            }
        }
    }
}