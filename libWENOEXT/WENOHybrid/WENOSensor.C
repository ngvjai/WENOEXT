/*---------------------------------------------------------------------------*\
  =========                 |
  \\      /  F ield         | OpenFOAM: The Open Source CFD Toolbox
   \\    /   O peration     |
    \\  /    A nd           | Copyright (C) 2011-2013 OpenFOAM Foundation
     \\/     M anipulation  |
-------------------------------------------------------------------------------
License
    This file is part of OpenFOAM.

    OpenFOAM is free software: you can redistribute it and/or modify it
    under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    OpenFOAM is distributed in the hope that it will be useful, but WITHOUT
    ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
    FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
    for more details.

    You should have received a copy of the GNU General Public License
    along with OpenFOAM.  If not, see <http://www.gnu.org/licenses/>.

Author
    Jan Wilhelm Gärtner <jan.gaertner@outlook.de> Copyright (C) 2020
    Tobias Martin, <tobimartin2@googlemail.com>.  All rights reserved.

\*---------------------------------------------------------------------------*/

#include "codeRules.H"
#include "WENOSensor.H"
#include "DynamicField.H"

#include "processorFvPatch.H"



// * * * * * * * * * * * * * * * * * Constructor * * * * * * * * * * * * * * //

template<class Type>
Foam::WENOSensor<Type>::WENOSensor
(
    const fvMesh& mesh,
    const label polOrder
)
:
    WENOCoeff<Type>(mesh,polOrder)
{
    // Read expert factors
    IOdictionary WENODict
    (
        IOobject
        (
            "WENODict",
            mesh.time().path()/"system",
            mesh,
            IOobject::READ_IF_PRESENT,
            IOobject::NO_WRITE
        )
    );

    theta_ = WENODict.lookupOrAddDefault<scalar>("theta", 1.0);
    
}


// * * * * * * * * * * * * * * * Member Functions  * * * * * * * * * * * * * //


// Specialisation for scalar
template<>
inline void Foam::WENOSensor<Foam::scalar>::calcWeight
(
    Field<scalar>& coeffsWeightedI,
    const label cellI,
    const GeometricField<scalar, fvPatchField, volMesh>& vf,
    const List<coeffType>& coeffsI
) const
{
    scalar gamma = 0.0;
    scalar gammaSum = 0.0;

    volScalarField& WENOShockSensor = 
        WENOCoeff<scalar>::storeOrRetrieve("WENOShockSensor");

    List<scalar> smoothIndList(coeffsI.size());    
    
    forAll(coeffsI, stencilI)
    {
        const auto& coeffsIsI = coeffsI[stencilI];

        // Get smoothness indicator
        const scalar smoothInd = trans(coeffsIsI) * (WENOBase_.B()[cellI]*coeffsIsI);
        
        smoothIndList[stencilI] = smoothInd;

        // Calculate gamma for central and sectorial stencils

        if (stencilI == 0)
        {
            gamma = dm_/(pow(this->epsilon_ + smoothInd,this->p_));
        }
        else
        {
            gamma = 1.0/(pow(this->epsilon_ + smoothInd,this->p_));
        }
        
        gammaSum += gamma;


        forAllU(coeffsIsI, coeffI)
        {
            coeffsWeightedI[coeffI] += coeffsIsI[coeffI]*gamma;
        }
    }

    forAll(coeffsWeightedI, coeffI)
    {
        coeffsWeightedI[coeffI] /= gammaSum;
    }

    
    WENOShockSensor[cellI] = max(smoothIndList);
}



template<class Type>
void Foam::WENOSensor<Type>::calcWeight
(
    Field<Type>& coeffsWeightedI,
    const label cellI,
    const GeometricField<Type, fvPatchField, volMesh>& vf,
    const List<coeffType>& coeffsI
) const 
{
    scalar gamma = 0.0;

    GeometricField<Type,fvPatchField,volMesh>& WENOShockSensor = 
        WENOCoeff<Type>::storeOrRetrieve("WENOShockSensor");

    for (label compI = 0; compI < vf[0].size(); compI++)
    {
        scalar gammaSum = 0.0;

        List<scalar> smoothIndList(coeffsI.size());
        
        forAll(coeffsI, stencilI)
        {
            const auto& coeffsIsI = coeffsI[stencilI];

            // Get smoothness indicator

            scalar smoothInd = 0.0;

            forAllU(coeffsIsI, coeffP)
            {
                scalar sumB = 0.0;

                forAllU(coeffsIsI, coeffQ)
                {
                    sumB +=
                    (
                        this->WENOBase_.B()[cellI](coeffP,coeffQ)
                       *coeffsIsI[coeffQ][compI]
                    );
                }

                smoothInd += coeffsIsI[coeffP][compI]*sumB;
            }

            smoothIndList = smoothInd;

            // Calculate gamma for central and sectorial stencils

            if (stencilI == 0)
            {
                gamma = this->dm_/(pow(this->epsilon_ + smoothInd,this->p_));
            }
            else
            {
                gamma = 1.0/(pow(this->epsilon_ + smoothInd,this->p_));
            }
            
            gammaSum += gamma;

            forAllU(coeffsIsI, coeffI)
            {
                coeffsWeightedI[coeffI][compI] += coeffsIsI[coeffI][compI]*gamma;
            }
        }

        forAll(coeffsWeightedI, coeffI)
        {
            coeffsWeightedI[coeffI][compI] /= gammaSum;
        }
        
        WENOShockSensor[cellI][compI] = max(smoothIndList);
    }
}



template<class Type>
Foam::GeometricField<Type, Foam::fvPatchField, Foam::volMesh>& 
Foam::WENOSensor<Type>::getShockSensor() const
{
    return WENOCoeff<Type>::storeOrRetrieve("WENOShockSensor");
    
}

// ************************************************************************* //
